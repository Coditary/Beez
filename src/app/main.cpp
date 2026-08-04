#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/phase_argument_parser.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

namespace
{

struct CliOptions
{
    std::optional<std::string> stepName;
    std::optional<beez::core::PhaseRequest> phaseRequest;
    std::optional<std::string> commandName;
};

// NOLINTNEXTLINE(modernize-avoid-c-arrays) -- standard main argv signature
[[nodiscard]] std::optional<CliOptions> parseArguments(int argc, const char* argv[])
{
    CliOptions options;
    int index = 1;

    while (index < argc)
    {
        const std::string Argument(argv[index]);

        if (Argument == "-p")
        {
            ++index;
            if (index >= argc)
            {
                return std::nullopt;
            }

            const auto Parsed = beez::core::parsePhaseArgument(argv[index]);
            if (!Parsed)
            {
                return std::nullopt;
            }

            options.phaseRequest = Parsed;
            ++index;
            continue;
        }

        if (Argument == "-s")
        {
            ++index;
            if (index >= argc)
            {
                return std::nullopt;
            }

            options.stepName = argv[index];
            ++index;
            continue;
        }

        if (options.commandName.has_value())
        {
            return std::nullopt;
        }

        options.commandName = Argument;
        ++index;
    }

    if (!options.stepName.has_value() && !options.phaseRequest.has_value() &&
        !options.commandName.has_value())
    {
        return std::nullopt;
    }

    return options;
}

void printUsage()
{
    std::cerr << "Usage: beez <task|workflow>\n";
    std::cerr << "       beez -p <phase>[:scope1,scope2]\n";
    std::cerr << "       beez -p <phase>[\"scope1\",\"scope2\"]\n";
    std::cerr << "       beez -s <step>\n";
}

}  // namespace

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

int main(int argc, const char* argv[])
{
    try
    {
        if (argc < 2)
        {
            printUsage();
            return 1;
        }

        const auto Options = parseArguments(argc, argv);
        if (!Options)
        {
            printUsage();
            return 1;
        }

        beez::core::Context context;
        beez::core::Registry registry;
        beez::plugin::PluginHost pluginHost;

        pluginHost.addPlugin(std::make_unique<beez::plugin::lua::LuaDslPlugin>());
        pluginHost.addPlugin(std::make_unique<beez::plugin::shell::ShellPlugin>());
        pluginHost.initialize(registry, context);

        beez::core::Orchestrator orchestrator(registry, context, pluginHost);

        const auto LoadResult = orchestrator.loadBuildScript();
        if (!LoadResult)
        {
            std::cerr << "Error: " << beez::core::toString(LoadResult.error()) << '\n';
            return 1;
        }

        if (Options->stepName)
        {
            const auto RunResult = orchestrator.runStep(*Options->stepName);
            if (!RunResult)
            {
                std::cerr << "Error: " << beez::core::toString(RunResult.error()) << '\n';
                return 1;
            }

            return RunResult.value();
        }

        if (Options->phaseRequest)
        {
            const auto RunResult = orchestrator.runPhase(*Options->phaseRequest);
            if (!RunResult)
            {
                std::cerr << "Error: " << beez::core::toString(RunResult.error()) << '\n';
                return 1;
            }

            return RunResult.value();
        }

        if (!Options->commandName.has_value())
        {
            printUsage();
            return 1;
        }

        const auto RunResult = orchestrator.run(*Options->commandName);
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
