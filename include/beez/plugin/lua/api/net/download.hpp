#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

#include <memory>

namespace beez::plugin::lua
{

void bindDownload(sol::table& netTable,
                  const std::shared_ptr<sol::state>& luaState,
                  const core::Context& context);

}  // namespace beez::plugin::lua
