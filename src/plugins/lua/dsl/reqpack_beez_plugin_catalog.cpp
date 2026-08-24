#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::plugin::lua
{

// NOLINTBEGIN(misc-include-cleaner)

void ReqpackBeezPluginCatalog::set(const std::vector<BeezPluginRef>& pluginRefs)
{
    plugins_ = pluginRefs;
}

void ReqpackBeezPluginCatalog::add(BeezPluginRef plugin)
{
    plugins_.push_back(std::move(plugin));
}

void ReqpackBeezPluginCatalog::merge(const std::vector<BeezPluginRef>& pluginRefs)
{
    for (const auto& pluginRef : pluginRefs)
    {
        const auto Existing = std::ranges::find_if(
            plugins_,
            [&](const BeezPluginRef& entry)
            {
                return entry.organization == pluginRef.organization && entry.name == pluginRef.name;
            });
        if (Existing != plugins_.end())
        {
            *Existing = pluginRef;
            continue;
        }

        plugins_.push_back(pluginRef);
    }
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

// NOLINTEND(misc-include-cleaner)

}  // namespace beez::plugin::lua
