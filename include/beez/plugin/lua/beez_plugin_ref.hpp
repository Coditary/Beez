#pragma once

#include <optional>
#include <string>

namespace beez::plugin::lua
{

struct BeezPluginRef
{
    std::string organization;
    std::string name;
    std::optional<std::string> path;
    std::optional<std::string> version;
    std::optional<std::string> source;
    std::optional<std::string> profile;
    bool fromInstalledCache = false;

    [[nodiscard]] bool isLocal() const
    {
        return path.has_value() && !path->empty();
    }

    [[nodiscard]] bool isRemote() const
    {
        return !isLocal();
    }
};

}  // namespace beez::plugin::lua
