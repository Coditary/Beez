#include "beez/cli/cli_app.hpp"
#include "beez/cli/list_formatter.hpp"
#include "beez/cli/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/core/run_options.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/logging/spdlog_backend.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <system_error>

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

        beez::core::Context context;

        if (Parsed.options.cleanCache)
        {
            const auto CachePath = context.projectRoot() / ".cache";
            std::error_code errorCode;
            std::filesystem::remove_all(CachePath, errorCode);
            std::cout << "Removed Beez cache: " << CachePath << '\n';
        }

        const bool hasRunTarget =
            Parsed.options.target.has_value() || Parsed.options.phaseRequest.has_value() ||
            Parsed.options.stepName.has_value() || Parsed.options.listKind.has_value();

        if (!hasRunTarget)
        {
            return 0;
        }

        beez::core::Registry registry;
        beez::plugin::PluginHost pluginHost;

        pluginHost.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
        pluginHost.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
        pluginHost.initialize(registry, context);

        const auto OutputMode = Parsed.options.verbose ? beez::logging::OutputMode::Verbose
                                                       : beez::logging::OutputMode::Clean;
        auto logger = beez::logging::createSpdlogLogger(OutputMode);

        const beez::core::RunOptions Options {
            .dryRun = Parsed.options.dryRun,
            .enableCache = Parsed.options.enableCache,
            .outputMode = OutputMode,
            .logger = logger.get(),
        };

        beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

        const auto LoadResult = orchestrator.loadBuildScript();
        if (!LoadResult)
        {
            std::cerr << "Error: " << beez::core::toString(LoadResult.error()) << '\n';
            return 1;
        }

        if (Parsed.options.listKind.has_value())
        {
            const auto Names = beez::cli::collectEntityNames(registry, *Parsed.options.listKind);
            std::cout << beez::cli::formatEntityList(*Parsed.options.listKind, Names);
            return 0;
        }

        if (Parsed.options.stepName)
        {
            const auto RunResult = orchestrator.runStep(*Parsed.options.stepName);
            if (!RunResult)
            {
                std::cerr << "Error: " << beez::core::toString(RunResult.error()) << '\n';
                return 1;
            }

            return RunResult.value();
        }

        if (Parsed.options.phaseRequest)
        {
            const auto RunResult = orchestrator.runPhase(*Parsed.options.phaseRequest);
            if (!RunResult)
            {
                std::cerr << "Error: " << beez::core::toString(RunResult.error()) << '\n';
                return 1;
            }

            return RunResult.value();
        }

        if (!Parsed.options.target.has_value())
        {
            std::cerr << beez::cli::CliApp::helpText();
            return 1;
        }

        const auto RunResult = orchestrator.run(*Parsed.options.target);
        if (!RunResult)
        {
            std::cerr << "Error: " << beez::core::toString(RunResult.error()) << '\n';
            return 1;
        }

        return RunResult.value();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
