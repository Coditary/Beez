#pragma once

#include "beez/core/runtime/context.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindDeserializeFile(sol::table& dataTable,
                         const std::shared_ptr<sol::state>& luaState,
                         const core::Context& context);
void bindSerializeFile(sol::table& dataTable, const core::Context& context);
void bindDeserializeString(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState);
void bindSerializeString(sol::table& dataTable);
void bindMerge(sol::table& dataTable);
void bindClone(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState);
void bindGet(sol::table& dataTable);
void bindSet(sol::table& dataTable);
void bindDiff(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState);
void bindValidate(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState);

}  // namespace beez::plugin::lua
