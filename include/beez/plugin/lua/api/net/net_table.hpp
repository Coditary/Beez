#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

#include <memory>

namespace beez::plugin::lua
{

sol::table bindNet(const std::shared_ptr<sol::state>& luaState, const core::Context& context);

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
