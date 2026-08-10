#pragma once

#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindInfo(sol::table& dateTable, const std::shared_ptr<sol::state>& luaState);

}  // namespace beez::plugin::lua
