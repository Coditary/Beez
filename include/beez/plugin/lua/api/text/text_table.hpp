#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#include <memory>

namespace beez::plugin::lua
{

sol::table bindText(const std::shared_ptr<sol::state>& luaState);

}  // namespace beez::plugin::lua
