#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

#include <memory>

namespace beez::plugin::lua
{

void bindIncrement(const std::shared_ptr<sol::state>& luaState, sol::table& beezTable);

}  // namespace beez::plugin::lua
