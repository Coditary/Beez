#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/beez_plugin_ref.hpp"

#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

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
