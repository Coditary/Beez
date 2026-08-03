#pragma once

#include <string>

namespace beez::plugin
{
class PluginHost;
}  // namespace beez::plugin

namespace beez::plugin
{

class IPlugin
{
  public:
    virtual ~IPlugin() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    virtual void registerCapabilities(PluginHost& host) = 0;
};

}  // namespace beez::plugin
