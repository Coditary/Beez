#pragma once

#include "beez/plugin/dsl_loader.hpp"
#include "beez/plugin/plugin.hpp"

#include <string>

namespace beez::plugin::lua
{

class LuaDslLoader : public IDslLoader
{
  public:
    bool load(const core::Context& context, core::Registry& registry) override;
};

class LuaDslPlugin : public IPlugin
{
  public:
    [[nodiscard]] std::string name() const override;
    void registerCapabilities(PluginHost& host) override;
};

}  // namespace beez::plugin::lua
