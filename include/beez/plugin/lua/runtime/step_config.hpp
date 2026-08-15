#pragma once

#include "beez/core/model/step_config.hpp"
#include "beez/core/runtime/context.hpp"

#include <functional>
#include <memory>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

struct LuaStepConfigOptions
{
    // NOLINTBEGIN(misc-include-cleaner)
    sol::protected_function finalize;
    sol::table schema;
    // NOLINTEND(misc-include-cleaner)
};

// NOLINTBEGIN(misc-include-cleaner)
[[nodiscard]] core::StepConfigPtr makeLuaStepConfig(const std::shared_ptr<sol::state>& luaState,
                                                    const sol::table& configTable);

[[nodiscard]] core::StepConfigPtr makeLuaStepConfig(const std::shared_ptr<sol::state>& luaState,
                                                    std::function<sol::table()> lazyBuilder,
                                                    LuaStepConfigOptions options = {});

[[nodiscard]] sol::table bindStepContext(const std::shared_ptr<sol::state>& luaState,
                                         const core::Context& context);
// NOLINTEND(misc-include-cleaner)

}  // namespace beez::plugin::lua
