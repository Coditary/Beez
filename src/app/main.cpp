#include "beez/cli/cli_app.hpp"
#include "beez/cli/install_completion.hpp"
#include "beez/cli/parsed_options.hpp"
#include "beez/cli/run_target.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/cache_storage.hpp"
#include "beez/core/config_paths.hpp"
#include "beez/core/config_schema.hpp"
#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/core/run_options.hpp"
#include "beez/core/settings.hpp"
#include "beez/core/settings_report.hpp"
#include "beez/logging/logging_settings.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/logging/run_log_writer.hpp"
#include "beez/logging/spdlog_backend.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/lua/lua_settings.hpp"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <system_error>

namespace
{

[[nodiscard]] beez::plugin::lua::LuaDslLoader*
findLuaDslLoader(beez::plugin::PluginHost& pluginHost)
{
    auto* loader = pluginHost.dslLoader();
    if (loader == nullptr)
    {
        return nullptr;
    }

    return dynamic_cast<beez::plugin::lua::LuaDslLoader*>(loader);
}

[[nodiscard]] std::optional<int> handleEarlyCliRequests(const beez::cli::ParsedOptions& options,
                                                        const char* argv0)
{
    if (options.installCompletion)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return beez::cli::runInstallCompletion(argv0 != nullptr ? argv0 : nullptr);
    }

    if (options.completeConfigOptions)
    {
        for (const auto& path :
             beez::core::listConfigOptionCompletions(options.completeConfigOptionsPrefix))
        {
            std::cout << path << '\n';
        }
        return 0;
    }

    if (options.dumpCompletion)
    {
        const auto Script = beez::cli::dumpCompletionScript(options.dumpCompletionShell);
        if (!Script.has_value())
        {
            std::cerr << "Error: unknown shell for --dump-completion: "
                      << options.dumpCompletionShell << '\n';
            return 1;
        }

        std::cout << *Script;
        return 0;
    }

    if (options.configOptions)
    {
        const auto Output = beez::core::formatConfigOptions(options.configOptionsPath);
        if (!Output.has_value())
        {
            std::cerr << "Error: unknown config option path: " << options.configOptionsPath << '\n';
            return 1;
        }

        std::cout << *Output << '\n';
        return 0;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<int> loadStartupSettings(const beez::cli::ParsedOptions& options,
                                                     beez::core::Context& context,
                                                     beez::core::BeezSettings& globalSettings,
                                                     std::filesystem::path& configPath)
{
    if (options.buildFile.has_value())
    {
        context.setBuildScriptPath(
            beez::core::resolveProjectRelativePath(context.projectRoot(), *options.buildFile));
    }

    configPath =
        options.configFile.has_value()
            ? beez::core::resolveProjectRelativePath(context.projectRoot(), *options.configFile)
            : beez::core::globalBeezConfigPath();
    if (options.configFile.has_value())
    {
        if (!std::filesystem::exists(configPath))
        {
            std::cerr << "Error: config file not found: " << configPath << '\n';
            return 1;
        }

        static_cast<void>(beez::plugin::lua::loadSettingsFromLuaFile(configPath, globalSettings));
        return std::nullopt;
    }

    beez::plugin::lua::tryLoadGlobalBeezSettings(globalSettings);
    return std::nullopt;
}

}  // namespace

int main(int argc, const char* argv[])
{
    try
    {
        const auto Parsed = beez::cli::CliApp::parse(argc, argv);
        if (Parsed.reason == beez::cli::CliExitReason::Help)
        {
            std::cout << beez::cli::CliApp::helpText();
            return Parsed.exitCode;
        }

        if (Parsed.reason == beez::cli::CliExitReason::Version)
        {
            std::cout << beez::cli::CliApp::versionText() << '\n';
            return Parsed.exitCode;
        }

        if (Parsed.reason == beez::cli::CliExitReason::Error)
        {
            std::cerr << beez::cli::CliApp::helpText();
            return Parsed.exitCode != 0 ? Parsed.exitCode : 1;
        }

        const char* programPath = nullptr;
        if (argc > 0)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            programPath = argv[0];
        }

        if (const auto EarlyExit = handleEarlyCliRequests(Parsed.options, programPath))
        {
            return *EarlyExit;
        }

        beez::core::Context context;
        beez::core::BeezSettings globalSettings;
        std::filesystem::path configPath;
        if (const auto StartupError =
                loadStartupSettings(Parsed.options, context, globalSettings, configPath))
        {
            return *StartupError;
        }

        beez::core::BeezSettings settings = globalSettings;
        settings.applyEnvironment(context);

        const bool HasRunTarget =
            Parsed.options.target.has_value() || Parsed.options.phaseRequest.has_value() ||
            Parsed.options.stepName.has_value() || Parsed.options.listKind.has_value() ||
            Parsed.options.cleanCache || Parsed.options.updateCache || Parsed.options.showConfig ||
            Parsed.options.configOptions;

        if (!HasRunTarget)
        {
            return 0;
        }

        beez::core::Registry registry;
        beez::plugin::PluginHost pluginHost;

        pluginHost.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
        pluginHost.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
        pluginHost.initialize(registry, context);

        beez::plugin::lua::LuaDslLoader* luaLoader = findLuaDslLoader(pluginHost);
        if (luaLoader == nullptr)
        {
            std::cerr << "Error: lua DSL loader is not available\n";
            return 1;
        }

        if (!std::filesystem::exists(context.buildScriptPath()))
        {
            std::cerr << "Error: build script not found: " << context.buildScriptPath() << '\n';
            return 1;
        }

        if (!luaLoader->load(context, registry))
        {
            std::cerr << "Error: failed to load build script: " << context.buildScriptPath()
                      << '\n';
            return 1;
        }

        settings.merge(luaLoader->buildSettings());
        const beez::core::BeezSettings ProjectSettings = luaLoader->buildSettings();
        settings.applyEnvironment(context);
        settings.applyCliOverrides(Parsed.options);

        if (Parsed.options.showConfig)
        {
            const beez::core::SettingsReportInput ReportInput {
                .globalSettings = globalSettings,
                .globalConfigPath = configPath,
                .projectSettings = ProjectSettings,
                .activeSettings = settings,
                .cliOptions = Parsed.options,
                .context = context,
            };
            std::cout << beez::core::formatActiveConfiguration(ReportInput) << '\n';
            return 0;
        }

        if (Parsed.options.cleanCache)
        {
            const auto CachePath = settings.resolveCacheOptions(context).root;
            std::error_code errorCode;
            std::filesystem::remove_all(CachePath, errorCode);
            std::cout << "Removed Beez cache: " << CachePath << '\n';
        }

        if (Parsed.options.updateCache)
        {
            const auto CacheOptions = settings.resolveCacheOptions(context);
            const std::size_t MigratedFiles = beez::core::updateCacheStorage(CacheOptions);
            std::cout << "Updated Beez cache: " << CacheOptions.root << " (";
            std::cout << MigratedFiles << " file";
            if (MigratedFiles != 1U)
            {
                std::cout << 's';
            }
            std::cout << " recompressed to "
                      << beez::core::toString(CacheOptions.compress.algorithm) << ", level "
                      << CacheOptions.compress.level << ", mode "
                      << beez::core::toString(CacheOptions.compress.mode) << ")\n";
        }

        const bool ShouldRunTarget =
            Parsed.options.target.has_value() || Parsed.options.phaseRequest.has_value() ||
            Parsed.options.stepName.has_value() || Parsed.options.listKind.has_value();
        if (!ShouldRunTarget)
        {
            return 0;
        }

        const auto OutputMode = settings.ui.outputMode.value_or(beez::logging::OutputMode::Clean);
        const auto UiSettings = settings.resolveUiSettings();
        const auto LoggingSettings = settings.resolveLoggingSettings(context);
        auto logger = beez::logging::createSpdlogLogger(OutputMode, UiSettings, LoggingSettings);

        std::unique_ptr<beez::logging::RunLogWriter> runLogWriter;
        if (LoggingSettings.workers != beez::logging::WorkerLogMode::Off)
        {
            runLogWriter = std::make_unique<beez::logging::RunLogWriter>(LoggingSettings);
        }

        beez::core::RunOptions runOptions = settings.toRunOptions(logger.get(), context);
        runOptions.runLogWriter = runLogWriter.get();

        beez::core::Orchestrator orchestrator(registry, context, pluginHost, runOptions);

        return beez::cli::runParsedInvocation(orchestrator, registry, Parsed.options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
