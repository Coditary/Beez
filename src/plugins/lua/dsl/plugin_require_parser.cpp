#include "beez/plugin/lua/dsl/plugin_loader.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] BeezPluginRef parsePluginRefEntry(const sol::object& entry)
{
    if (!entry.is<sol::table>())
    {
        throw std::runtime_error("beez plugin entries must be tables with name and version");
    }

    const sol::table PluginTable = entry.as<sol::table>();
    const sol::object NameObject = PluginTable["name"];
    if (!NameObject.is<std::string>())
    {
        throw std::runtime_error("beez plugin entry requires string name");
    }

    const sol::object VersionObject = PluginTable["version"];
    if (!VersionObject.is<std::string>())
    {
        throw std::runtime_error("beez plugin entry requires string version");
    }

    return {.name = NameObject.as<std::string>(), .version = VersionObject.as<std::string>()};
}

}  // namespace

std::vector<BeezPluginRef> parseBeezRequireTable(const sol::table& table)
{
    const sol::object BeezObject = table["beez"];
    if (!BeezObject.valid() || !BeezObject.is<sol::table>())
    {
        throw std::runtime_error("require table must contain a beez array of plugins");
    }

    std::vector<BeezPluginRef> plugins;
    const sol::table BeezTable = BeezObject.as<sol::table>();
    for (const auto& [key, value] : BeezTable)
    {
        if (!key.is<int>())
        {
            throw std::runtime_error("require.beez must be an array of plugin references");
        }
        plugins.push_back(parsePluginRefEntry(value));
    }
    return plugins;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
