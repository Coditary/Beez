#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <algorithm>
#include <ranges>

namespace beez::plugin::lua
{

void ReqpackBeezPluginCatalog::set(const std::vector<BeezPluginRef>& pluginRefs)
{
    plugins_ = pluginRefs;
}

void ReqpackBeezPluginCatalog::add(BeezPluginRef plugin)
{
    plugins_.push_back(std::move(plugin));
}

std::optional<BeezPluginRef> ReqpackBeezPluginCatalog::find(const std::string& organization,
                                                            const std::string& plugin) const
{
    const auto Match = std::ranges::find_if(
        plugins_,
        [&](const BeezPluginRef& entry)
        { return entry.organization == organization && entry.name == plugin; });
    if (Match == plugins_.end())
    {
        return std::nullopt;
    }

    return *Match;
}

}  // namespace beez::plugin::lua
