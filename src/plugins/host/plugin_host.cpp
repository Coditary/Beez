#include "beez/plugin/host/plugin_host.hpp"

#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/contract/executor.hpp"
#include "beez/plugin/contract/plugin.hpp"

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

void PluginHost::setExecutor(std::unique_ptr<IExecutor> newExecutor)
{
    executor_ = std::move(newExecutor);
}

void PluginHost::setDslLoader(std::unique_ptr<IDslLoader> newDslLoader)
{
    dslLoader_ = std::move(newDslLoader);
}

}  // namespace beez::plugin
