#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindPid(sol::table& sysTable);

}  // namespace beez::plugin::lua
