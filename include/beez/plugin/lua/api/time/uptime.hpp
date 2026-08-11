#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindUptime(sol::table& timeTable);

}  // namespace beez::plugin::lua
