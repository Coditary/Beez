#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void bindGlob(sol::table& fsTable,
              const std::shared_ptr<sol::state>& luaState,
              const core::Context& context);

}  // namespace beez::plugin::lua
