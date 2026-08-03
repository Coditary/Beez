#include "beez/plugin/shell/shell_executor.h"

#include "beez/core/context.h"
#include "beez/plugin/plugin_host.h"

#include <cstdlib>
#include <memory>
#include <string>

namespace beez::plugin::shell
{

namespace
{

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char Character : value)
    {
        if (Character == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += Character;
        }
    }
    quoted += '\'';
    return quoted;
}

}  // namespace

int ShellExecutor::execute(const std::string& command, const core::Context& context)
{
    const std::string WrappedCommand =
        "cd " + shellQuote(context.projectRoot().string()) + " && " + command;
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    return std::system(WrappedCommand.c_str());
}

std::string ShellPlugin::name() const
{
    return "shell";
}

void ShellPlugin::registerCapabilities(PluginHost& host)
{
    host.setExecutor(std::make_unique<ShellExecutor>());
}

}  // namespace beez::plugin::shell
