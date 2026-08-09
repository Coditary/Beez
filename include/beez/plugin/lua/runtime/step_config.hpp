#pragma once

#include "beez/core/runtime/context.hpp"
#include "beez/core/model/step_config.hpp"

#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] core::StepConfigPtr makeLuaStepConfig(const std::shared_ptr<sol::state>& luaState,
                                                    const sol::table& configTable);

[[nodiscard]] sol::table bindStepContext(const std::shared_ptr<sol::state>& luaState,
                                         const core::Context& context);

}  // namespace beez::plugin::lua
