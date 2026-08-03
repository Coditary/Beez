#include "beez/plugin/shell/shell_executor.h"

#include "beez/plugin/plugin_host.h"

#include <cstdlib>
#include <memory>
#include <string>

namespace beez::plugin::shell
{

int ShellExecutor::execute(const std::string& command, const core::Context& /*context*/)
{
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe)
    return std::system(command.c_str());
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
