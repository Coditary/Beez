#include "beez/cli/session.hpp"

#include "beez/cli/commands/run.hpp"
#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config_paths.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/run_options.hpp"
#include "beez/logging/backends/spdlog_logger.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/persistence/run_log_writer.hpp"
#include "beez/logging/settings/logging_settings.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/lua/lua_settings.hpp"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

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

void loadGlobalSettings(LoadedProject& project)
{
    project.configPath = core::globalBeezConfigPath();
    plugin::lua::tryLoadGlobalBeezSettings(project.globalSettings);
    project.settings = project.globalSettings;
    project.settings.applyEnvironment(project.context);
}

std::optional<int> loadBuildScript(LoadedProject& project, bool silentRun)
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

    if (!std::filesystem::exists(project.context.buildScriptPath()))
    {
        writeErrorUnlessSilent(silentRun,
                               [&project]()
                               {
                                   std::cerr << "Error: build script not found: "
                                             << project.context.buildScriptPath() << '\n';
                               });
        return 1;
    }

    if (!project.luaLoader->load(project.context, project.registry))
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
    return runOrchestratorCommand(orchestrator, project.registry, options, OutputMode);
}

}  // namespace beez::cli
