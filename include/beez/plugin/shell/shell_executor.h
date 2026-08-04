#pragma once

#include "beez/plugin/executor.hpp"
#include "beez/plugin/plugin.hpp"

#include <string>

namespace beez::plugin::shell
{

class ShellExecutor : public IExecutor
{
  public:
    int execute(const std::string& command, const core::Context& context) override;
};

class ShellPlugin : public IPlugin
{
  public:
    [[nodiscard]] std::string name() const override;
    void registerCapabilities(PluginHost& host) override;
};

}  // namespace beez::plugin::shell
