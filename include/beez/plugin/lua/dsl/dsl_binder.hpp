#pragma once

#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/core/settings.hpp"

#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void registerDsl(const std::shared_ptr<sol::state>& luaState,
                 core::Registry& registry,
                 const core::Context& context,
                 core::BeezSettings& buildSettings);

}  // namespace beez::plugin::lua
