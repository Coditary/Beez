#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] std::vector<std::string> parseTaskStepReferences(const sol::table& stepTable);

void rejectDeprecatedTaskFields(const sol::table& stepTable);

void validateTaskPluginStepReference(const std::string& taskName,
                                     const std::string& stepReference,
                                     const core::Registry& registry,
                                     const ReqpackBeezPluginCatalog& catalog);

}  // namespace beez::plugin::lua
