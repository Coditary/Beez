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
    IPlugin() = default;
    virtual ~IPlugin() = default;
    IPlugin(const IPlugin&) = delete;
    IPlugin& operator=(const IPlugin&) = delete;
    IPlugin(IPlugin&&) = delete;
    IPlugin& operator=(IPlugin&&) = delete;

    [[nodiscard]] virtual std::string name() const = 0;
    virtual void registerCapabilities(PluginHost& host) = 0;
};

}  // namespace beez::plugin
