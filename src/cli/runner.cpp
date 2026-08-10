#include "beez/cli/runner.hpp"

#include "beez/cli/commands/cache.hpp"
#include "beez/cli/commands/config.hpp"
#include "beez/cli/commands/list.hpp"
#include "beez/cli/completion/install_completion.hpp"
#include "beez/cli/parsing/cli_parser.hpp"
#include "beez/cli/parsing/help_text.hpp"
#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/cli/session.hpp"
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
           options.updateCache || options.showConfig || options.configOptions;
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

        LoadedProject project;
        loadGlobalSettings(project);

        if (!hasRunTarget(Parsed.options))
        {
            return 0;
        }

        if (const auto LoadError = loadBuildScript(project, silentRun))
        {
            return *LoadError;
        }

        mergeProjectSettings(project, Parsed.options);

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
