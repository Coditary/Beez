#include "beez/cli/cli_app.hpp"
#include "beez/cli/install_completion.hpp"
#include "beez/cli/parsed_options.hpp"
#include "beez/cli/run_target.hpp"
#include "beez/core/config_paths.hpp"
#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/core/run_options.hpp"
#include "beez/core/settings.hpp"
#include "beez/core/settings_report.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/logging/spdlog_backend.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/lua/lua_settings.hpp"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
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

        if (Parsed.options.installCompletion)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return beez::cli::runInstallCompletion(argc > 0 ? argv[0] : nullptr);
        }

        beez::core::BeezSettings globalSettings;
        beez::plugin::lua::tryLoadGlobalBeezSettings(globalSettings);

        beez::core::BeezSettings settings = globalSettings;
        settings.applyEnvironment();

        beez::core::Context context;
        settings.applyToContext(context);

        const bool HasRunTarget =
            Parsed.options.target.has_value() || Parsed.options.phaseRequest.has_value() ||
            Parsed.options.stepName.has_value() || Parsed.options.listKind.has_value() ||
            Parsed.options.cleanCache || Parsed.options.showConfig;

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
            std::cerr << "Error: build.lua not found\n";
            return 1;
        }

        if (!luaLoader->load(context, registry))
        {
            std::cerr << "Error: failed to load build.lua\n";
            return 1;
        }

        settings.merge(luaLoader->buildSettings());
        const beez::core::BeezSettings ProjectSettings = luaLoader->buildSettings();
        settings.applyToContext(context);
        settings.applyCliOverrides(Parsed.options);

        if (Parsed.options.showConfig)
        {
            const beez::core::SettingsReportInput ReportInput {
                .globalSettings = globalSettings,
                .globalConfigPath = beez::core::globalBeezConfigPath(),
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

        const bool ShouldRunTarget =
            Parsed.options.target.has_value() || Parsed.options.phaseRequest.has_value() ||
            Parsed.options.stepName.has_value() || Parsed.options.listKind.has_value();
        if (!ShouldRunTarget)
        {
            return 0;
        }

        const auto OutputMode = settings.ui.outputMode.value_or(beez::logging::OutputMode::Clean);
        auto logger = beez::logging::createSpdlogLogger(OutputMode);

        const beez::core::RunOptions Options = settings.toRunOptions(logger.get(), context);

        beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

        return beez::cli::runParsedInvocation(orchestrator, registry, Parsed.options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
