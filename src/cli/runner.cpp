#include "beez/cli/runner.hpp"

#include "beez/cli/commands/cache.hpp"
#include "beez/cli/commands/config.hpp"
#include "beez/cli/commands/list.hpp"
#include "beez/cli/commands/reqpack.hpp"
#include "beez/cli/completion/install_completion.hpp"
#include "beez/cli/parsing/cli_parser.hpp"
#include "beez/cli/parsing/help_text.hpp"
#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/cli/session.hpp"
#include "beez/cli/tempify_dispatch.hpp"
#include "beez/core/config/paths/bridge_paths.hpp"
#include "beez/core/config/report/settings_report.hpp"

#include <exception>
#include <functional>
#include <iostream>
#include <optional>

namespace beez::cli
{

namespace
{

void writeErrorUnlessSilent(bool silentRun, const std::function<void()>& writeError)
{
    if (!silentRun)
    {
        writeError();
    }
}

[[nodiscard]] std::optional<int> handleParseResult(const CliParseResult& parsed)
{
    if (parsed.reason == CliExitReason::Help)
    {
        std::cout << helpText();
        return parsed.exitCode;
    }

    if (parsed.reason == CliExitReason::Version)
    {
        std::cout << versionText() << '\n';
        return parsed.exitCode;
    }

    if (parsed.reason == CliExitReason::Error)
    {
        std::cerr << helpText();
        return parsed.exitCode != 0 ? parsed.exitCode : 1;
    }

    return std::nullopt;
}

[[nodiscard]] bool hasRunTarget(const ParsedOptions& options)
{
    return options.target.has_value() || options.phaseRequest.has_value() ||
           options.stepName.has_value() || options.listKind.has_value() || options.cleanCache ||
           options.updateCache || options.showConfig || options.configOptions ||
           options.installDependencies;
}

[[nodiscard]] bool shouldExecuteTarget(const ParsedOptions& options)
{
    return options.target.has_value() || options.phaseRequest.has_value() ||
           options.stepName.has_value() || options.listKind.has_value();
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays) -- CLI entry point
int run(int argc, const char* argv[])
{
    if (isInitMode(argc, argv))
    {
        return runTempifyInitMode(collectInitArgs(argc, argv));
    }

    bool silentRun = false;

    try
    {
        const auto Parsed = CliParser::parse(argc, argv);
        silentRun = Parsed.options.silent;

        if (const auto ParseExit = handleParseResult(Parsed))
        {
            return *ParseExit;
        }

        const char* programPath = nullptr;
        if (argc > 0)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            programPath = argv[0];
        }

        if (Parsed.options.installCompletion)
        {
            return runInstallCompletion(programPath);
        }

        if (const auto EarlyExit = runEarlyConfigCommands(Parsed.options))
        {
            return *EarlyExit;
        }

        if (Parsed.options.linkPath.has_value())
        {
            const auto Cwd = std::filesystem::current_path();
            const auto BuildScriptSource = Parsed.options.linkPath.has_value() && !Parsed.options.linkPath->empty()
                                              ? *Parsed.options.linkPath
                                              : Cwd / "build.lua";

            if (!std::filesystem::exists(BuildScriptSource))
            {
                std::cerr << "Error: build script not found: " << BuildScriptSource << '\n';
                return 1;
            }

            const auto Result = core::createBridgeLink(BuildScriptSource, Cwd);
            if (Result.alreadyExisted)
            {
                std::cout << "Warning: bridge already exists for " << Cwd << '\n';
            }
            else
            {
                std::cout << "Linked " << BuildScriptSource << " -> " << Result.bridgeDir << '\n';
            }
            std::cout << "Bridge: " << Result.bridgeDir / "build.lua" << '\n';
            return 0;
        }

        LoadedProject project;
        if (const auto LoadError = loadGlobalSettings(project, Parsed.options.profile))
        {
            return *LoadError;
        }

        if (!hasRunTarget(Parsed.options))
        {
            return 0;
        }

        if (const auto LoadError = loadBuildScript(project,
                                                   silentRun,
                                                   !Parsed.options.installDependencies,
                                                   Parsed.options.fromBridge ? ScriptSource::Bridge
                                                   : Parsed.options.fromGlobal
                                                       ? ScriptSource::Global
                                                       : ScriptSource::Auto))
        {
            return *LoadError;
        }

        mergeProjectSettings(project, Parsed.options);

        if (Parsed.options.installDependencies)
        {
            if (shouldExecuteTarget(Parsed.options))
            {
                writeErrorUnlessSilent(silentRun,
                                       []()
                                       {
                                           std::cerr
                                               << "Error: --install cannot be combined with a "
                                                  "task, workflow, phase, step, or --list\n";
                                       });
                return 1;
            }

            return runReqPackInstallCommand(project, Parsed.options).value_or(0);
        }

        if (const auto ReqPackError = ensureReqPackDependencies(project, Parsed.options))
        {
            return *ReqPackError;
        }

        const core::SettingsReportInput ReportInput {
            .globalSettings = project.globalSettings,
            .globalConfigPath = project.configPath,
            .projectSettings = project.projectSettings,
            .activeSettings = project.settings,
            .cliOptions = Parsed.options,
            .context = project.context,
        };

        if (const auto ShowConfigExit = runShowConfigCommand(Parsed.options, ReportInput))
        {
            return *ShowConfigExit;
        }

        runCacheMaintenance(Parsed.options, project.settings, project.context);

        if (!shouldExecuteTarget(Parsed.options))
        {
            return 0;
        }

        if (Parsed.options.listKind.has_value())
        {
            return runListCommand(project.registry, *Parsed.options.listKind);
        }

        return runWithOrchestrator(project, Parsed.options);
    }
    catch (const std::exception& error)
    {
        writeErrorUnlessSilent(
            silentRun, [&error]() { std::cerr << "Fatal error: " << error.what() << '\n'; });
        return 1;
    }
}

}  // namespace beez::cli
