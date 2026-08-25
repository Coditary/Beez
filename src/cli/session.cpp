#include "beez/cli/session.hpp"

#include "beez/cli/commands/run.hpp"
#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/paths/bridge_paths.hpp"
#include "beez/core/config/paths/config_paths.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/logging/backends/spdlog_logger.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/persistence/run_log_writer.hpp"
#include "beez/logging/settings/logging_settings.hpp"
#include "beez/plugin/host/plugin_host.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"
#include "beez/plugin/lua/settings/lua_settings.hpp"
#include "beez/plugin/shell/shell_executor.hpp"

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

namespace beez::cli
{

namespace
{

[[nodiscard]] plugin::lua::LuaDslLoader* findLuaDslLoader(plugin::PluginHost& pluginHost)
{
    auto* loader = pluginHost.dslLoader();
    if (loader == nullptr)
    {
        return nullptr;
    }

    return dynamic_cast<plugin::lua::LuaDslLoader*>(loader);
}

void writeErrorUnlessSilent(bool silentRun, const std::function<void()>& writeError)
{
    if (!silentRun)
    {
        writeError();
    }
}

}  // namespace

[[nodiscard]] std::optional<int> loadGlobalSettings(LoadedProject& project,
                                                   const std::optional<std::string>& profileName)
{
    project.configPath = core::globalBeezConfigPath();
    plugin::lua::tryLoadGlobalBeezSettings(project.globalSettings);

    if (profileName.has_value())
    {
        const auto ProfilePath = core::profileBeezConfigPath(*profileName);
        // Profiles are a pure runtime selection: a missing Lua profile file is
        // not an error. The name stays active for DSL profile filters and
        // parameters() profile lookups.
        if (!ProfilePath.empty() && std::filesystem::exists(ProfilePath))
        {
            plugin::lua::tryLoadProfileBeezSettings(*profileName, project.globalSettings);
        }
    }

    project.registry.setProfile(profileName);
    project.settings = project.globalSettings;
    project.settings.applyEnvironment(project.context);
    return std::nullopt;
}

std::optional<int> loadBuildScript(LoadedProject& project,
                                   bool silentRun,
                                   bool validateRegistry,
                                   ScriptSource source)
{
    project.pluginHost.addPlugin(std::make_unique<plugin::lua::LuaDslPlugin>());
    project.pluginHost.addPlugin(std::make_unique<plugin::shell::ShellPlugin>());
    project.pluginHost.initialize(project.registry, project.context);

    project.luaLoader = findLuaDslLoader(project.pluginHost);
    if (project.luaLoader == nullptr)
    {
        writeErrorUnlessSilent(silentRun,
                               []() { std::cerr << "Error: lua DSL loader is not available\n"; });
        return 1;
    }

    if (source == ScriptSource::Bridge)
    {
        const auto BridgePath = core::resolveBridge(project.context.projectRoot());
        if (!BridgePath.has_value())
        {
            writeErrorUnlessSilent(
                silentRun,
                [&project]()
                {
                    std::cerr << "Error: no bridge linked for project: "
                              << project.context.projectRoot() << '\n';
                });
            return 1;
        }
        project.context.setBuildScriptFileName(BridgePath->string());
    }
    else if (source == ScriptSource::Global)
    {
        const auto GlobalScript = core::globalBuildScriptPath();
        if (GlobalScript.empty() || !std::filesystem::exists(GlobalScript))
        {
            writeErrorUnlessSilent(silentRun,
                                   []()
                                   {
                                       std::cerr
                                           << "Error: global build script not found: "
                                           << core::globalBuildScriptPath().string() << '\n';
                                   });
            return 1;
        }
        project.context.setBuildScriptFileName(GlobalScript.string());
    }
    else if (!std::filesystem::exists(project.context.buildScriptPath()))
    {
        // Check bridge index for a linked build script
        const auto BridgePath = core::resolveBridge(project.context.projectRoot());
        if (BridgePath.has_value())
        {
            project.context.setBuildScriptFileName(BridgePath->string());
        }
        else
        {
            // Last resort: global build script shipped in the beez config directory
            const auto GlobalScript = core::globalBuildScriptPath();
            if (!GlobalScript.empty() && std::filesystem::exists(GlobalScript))
            {
                project.context.setBuildScriptFileName(GlobalScript.string());
            }
            else
            {
                writeErrorUnlessSilent(silentRun,
                                       [&project]()
                                       {
                                           std::cerr << "Error: build script not found: "
                                                     << project.context.buildScriptPath() << '\n';
                                       });
                return 1;
            }
        }
    }

    if (!project.luaLoader->load(project.context, project.registry, validateRegistry))
    {
        writeErrorUnlessSilent(silentRun,
                               [&project]()
                               {
                                   std::cerr << "Error: failed to load build script: "
                                             << project.context.buildScriptPath() << '\n';
                               });
        return 1;
    }

    return std::nullopt;
}

void mergeProjectSettings(LoadedProject& project, const ParsedOptions& options)
{
    project.settings.merge(project.luaLoader->buildSettings());
    project.projectSettings = project.luaLoader->buildSettings();
    project.settings.applyEnvironment(project.context);
    project.settings.applyCliOverrides(options);
}

int runWithOrchestrator(LoadedProject& project, const ParsedOptions& options)
{
    const auto OutputMode = project.settings.ui.outputMode.value_or(logging::OutputMode::Clean);
    const auto UiSettings = project.settings.resolveUiSettings();
    const auto LoggingSettings = project.settings.resolveLoggingSettings(project.context);
    auto logger = logging::createSpdlogLogger(OutputMode, UiSettings, LoggingSettings);

    std::unique_ptr<logging::RunLogWriter> runLogWriter;
    if (LoggingSettings.workers != logging::WorkerLogMode::Off)
    {
        runLogWriter = std::make_unique<logging::RunLogWriter>(LoggingSettings);
    }

    core::RunOptions runOptions = project.settings.toRunOptions(logger.get(), project.context);
    runOptions.runLogWriter = runLogWriter.get();

    core::Orchestrator orchestrator(
        project.registry, project.context, project.pluginHost, runOptions);
    return runOrchestratorCommand(
        orchestrator, project.registry, project.context, options, OutputMode);
}

}  // namespace beez::cli
