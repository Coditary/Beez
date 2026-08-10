#pragma once

#include "beez/plugin/contract/executor.hpp"
#include "beez/plugin/contract/plugin.hpp"

#include <string>

namespace beez::plugin::shell
{

class ShellExecutor : public IExecutor
{
  public:
    int execute(const std::string& command,
                const core::Context& context,
                std::string* capturedOutput = nullptr) override;
};

class ShellPlugin : public IPlugin
{
  public:
    [[nodiscard]] std::string name() const override;
    void registerCapabilities(PluginHost& host) override;
};

}  // namespace beez::plugin::shell
