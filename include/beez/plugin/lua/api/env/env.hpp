#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindEnvToTable(sol::table& beezTable, const core::Context& context);

}  // namespace beez::plugin::lua
