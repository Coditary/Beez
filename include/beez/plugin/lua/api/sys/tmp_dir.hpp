#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindTmpDir(sol::table& sysTable);

}  // namespace beez::plugin::lua
