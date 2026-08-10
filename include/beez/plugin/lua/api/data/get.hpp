#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindGet(sol::table& dataTable);

}  // namespace beez::plugin::lua
