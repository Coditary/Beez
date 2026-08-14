#pragma once

#include <functional>
#include <memory>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void parseConfigureTable(const sol::table& entriesTable,
                         const std::shared_ptr<sol::state>& luaState,
                         const std::function<void(const std::string& qualifiedPluginName,
                                                  const sol::table& pluginConfig)>& onPluginConfig,
                         const std::function<void(const std::string& stepName,
                                                  const sol::table& stepConfig)>& onStepConfig);

}  // namespace beez::plugin::lua
