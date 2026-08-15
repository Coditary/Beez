#pragma once

#include "beez/core/model/task_action.hpp"

#include <memory>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
#include <string>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] std::vector<core::TaskAction>
parseTaskActions(const sol::table& actionsTable, const std::shared_ptr<sol::state>& luaState);

[[nodiscard]] bool isTaskActionListTable(const sol::table& table);

[[nodiscard]] bool isPluginTaskImportTable(const sol::table& table);

[[nodiscard]] std::string buildPluginTaskReference(const sol::table& importTable);

}  // namespace beez::plugin::lua
