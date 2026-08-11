#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindRegexMatch(sol::table& textTable);

}  // namespace beez::plugin::lua
