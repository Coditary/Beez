#pragma once

#include "beez/core/config/settings/settings.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"

#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void registerBeezApi(const std::shared_ptr<sol::state>& luaState,
                     const core::Context& context,
                     core::BeezSettings& buildSettings);

}  // namespace beez::plugin::lua
