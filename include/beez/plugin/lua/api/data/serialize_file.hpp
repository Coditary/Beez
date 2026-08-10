#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindSerializeFile(sol::table& dataTable, const core::Context& context);

}  // namespace beez::plugin::lua
