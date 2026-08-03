#include "beez/plugin/plugin_host.h"

#include "beez/plugin/dsl_loader.hpp"
#include "beez/plugin/executor.hpp"
#include "beez/plugin/plugin.hpp"

#include <memory>
#include <utility>

namespace beez::plugin
{

void PluginHost::addPlugin(std::unique_ptr<IPlugin> plugin)
{
    plugins_.push_back(std::move(plugin));
}

void PluginHost::initialize(core::Registry& /*registry*/, core::Context& /*context*/)
{
    for (auto& plugin : plugins_)
    {
        plugin->registerCapabilities(*this);
    }
}

IExecutor* PluginHost::executor() const
{
    return executor_.get();
}

IDslLoader* PluginHost::dslLoader() const
{
    return dslLoader_.get();
}

void PluginHost::setExecutor(std::unique_ptr<IExecutor> executor)
{
    executor_ = std::move(executor);
}

void PluginHost::setDslLoader(std::unique_ptr<IDslLoader> dslLoader)
{
    dslLoader_ = std::move(dslLoader);
}

}  // namespace beez::plugin
