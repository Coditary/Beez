#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"

#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

struct BeezPluginRef
{
    std::string name;
    std::string version;
};

[[nodiscard]] std::vector<BeezPluginRef> parseBeezRequireTable(const sol::table& table);

void loadBeezPlugins(const std::vector<BeezPluginRef>& plugins,
                     core::Registry& registry,
                     const core::Context& context);

}  // namespace beez::plugin::lua
