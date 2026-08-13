#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void parseWorkflowsTable(const sol::table& workflowsTable,
                         core::Registry& registry,
                         const ReqpackBeezPluginCatalog* reqpackBeezPlugins);

}  // namespace beez::plugin::lua
