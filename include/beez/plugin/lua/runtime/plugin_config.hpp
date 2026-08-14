#pragma once

#include "beez/core/model/step_config.hpp"

#include <memory>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void clearPluginConfigRegistry();

void registerPluginConfigDefinition(const std::string& pluginKey,
                                    const std::shared_ptr<sol::state>& luaState,
                                    const sol::table& configTable);

[[nodiscard]] core::StepConfigPtr
makePluginStepConfig(const std::shared_ptr<sol::state>& luaState,
                     const std::string& pluginKey,
                     const sol::table& stepConfigTable);

}  // namespace beez::plugin::lua
