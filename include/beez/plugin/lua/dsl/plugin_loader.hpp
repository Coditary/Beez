#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"

#include <optional>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

struct BeezPluginRef
{
    std::string organization;
    std::string name;
    std::optional<std::string> path;
    std::optional<std::string> version;
    std::optional<std::string> source;
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

[[nodiscard]] std::vector<BeezPluginRef> parseBeezPluginTable(const sol::table& table);

void loadBeezPlugins(const std::vector<BeezPluginRef>& plugins,
                     core::Registry& registry,
                     const core::Context& context);

void loadInstalledBeezPlugin(const std::string& organization,
                             const std::string& name,
                             const std::string& version,
                             core::Registry& registry,
                             const core::Context& context);

bool tryLoadInstalledBeezPlugin(const std::string& organization,
                                const std::string& name,
                                const std::string& version,
                                core::Registry& registry,
                                const core::Context& context);

}  // namespace beez::plugin::lua
