#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

namespace beez::plugin::lua
{

void ReqpackBeezPluginCatalog::set(const std::vector<BeezPluginRef>& plugins)
{
    plugins_ = plugins;
}

void ReqpackBeezPluginCatalog::add(BeezPluginRef plugin)
{
    plugins_.push_back(std::move(plugin));
}

std::optional<BeezPluginRef> ReqpackBeezPluginCatalog::find(const std::string& organization,
                                                            const std::string& plugin) const
{
    for (const auto& entry : plugins_)
    {
        if (entry.organization == organization && entry.name == plugin)
        {
            return entry;
        }
    }

    return std::nullopt;
}

}  // namespace beez::plugin::lua
