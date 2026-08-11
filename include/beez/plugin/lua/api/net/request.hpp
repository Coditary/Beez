#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

#include <memory>

namespace beez::plugin::lua
{

void bindRequest(sol::table& netTable, const std::shared_ptr<sol::state>& luaState);

}  // namespace beez::plugin::lua
