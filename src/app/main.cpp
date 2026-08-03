#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"
#include "beez/plugin/plugin_host.h"
#include "beez/plugin/shell/shell_executor.h"

#include <exception>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: beez <task|workflow>\n";
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

        const auto RunResult = orchestrator.run(argv[1]);
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
