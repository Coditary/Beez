#include "beez/plugin/lua/dsl/reqpack_parser.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] core::ReqPackPackage parsePackageEntry(const sol::object& entry,
                                                     const std::string& plugin)
{
    if (entry.is<std::string>())
    {
        return {.name = entry.as<std::string>()};
    }

    if (!entry.is<sol::table>())
    {
        throw std::runtime_error("reqpack." + plugin +
                                 " entries must be strings or tables with name/version");
    }

    const sol::table PackageTable = entry.as<sol::table>();
    const sol::object NameObject = PackageTable["name"];
    if (!NameObject.is<std::string>())
    {
        throw std::runtime_error("reqpack." + plugin + " package table requires string name");
    }

    core::ReqPackPackage package {.name = NameObject.as<std::string>()};
    const sol::object VersionObject = PackageTable["version"];
    if (VersionObject.valid() && VersionObject.get_type() != sol::type::lua_nil)
    {
        if (!VersionObject.is<std::string>())
        {
            throw std::runtime_error("reqpack." + plugin + " package version must be a string");
        }
        package.version = VersionObject.as<std::string>();
    }

    return package;
}

[[nodiscard]] std::vector<core::ReqPackPackage> parsePluginPackages(const sol::table& pluginTable,
                                                                    const std::string& plugin)
{
    std::vector<core::ReqPackPackage> packages;
    for (const auto& [key, value] : pluginTable)
    {
        if (!key.is<int>())
        {
            throw std::runtime_error("reqpack." + plugin + " must be an array of packages");
        }
        packages.push_back(parsePackageEntry(value, plugin));
    }
    return packages;
}

}  // namespace

core::ReqPackManifest parseReqPackTable(const sol::table& table)
{
    core::ReqPackManifest manifest;
    for (const auto& [key, value] : table)
    {
        if (!key.is<std::string>())
        {
            throw std::runtime_error("reqpack keys must be plugin names (strings)");
        }

        const std::string Plugin = key.as<std::string>();
        if (Plugin == "beez")
        {
            continue;
        }

        if (!value.is<sol::table>())
        {
            throw std::runtime_error("reqpack." + Plugin + " must be an array of packages");
        }

        manifest.plugins.emplace(Plugin, parsePluginPackages(value.as<sol::table>(), Plugin));
    }
    return manifest;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
