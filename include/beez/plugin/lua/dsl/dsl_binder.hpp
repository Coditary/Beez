#pragma once

#include "beez/core/config/settings.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/registry/registry.hpp"

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
