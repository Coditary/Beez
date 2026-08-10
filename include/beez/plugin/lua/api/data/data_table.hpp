#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#include <memory>

namespace beez::plugin::lua
{

sol::table bindData(const std::shared_ptr<sol::state>& luaState, const core::Context& context);

}  // namespace beez::plugin::lua
