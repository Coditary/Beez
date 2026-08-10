#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

#include <string>
#include <vector>

namespace beez::plugin::lua::data_detail
{

void deepMerge(sol::table& target, const sol::table& source);
[[nodiscard]] sol::table cloneTable(sol::state_view luaState, const sol::table& table);
[[nodiscard]] sol::object
getPath(const sol::table& table, const std::string& path, const sol::object& defaultValue);
void setPath(sol::table& table, const std::string& path, const sol::object& value);
[[nodiscard]] sol::table
diffTables(sol::state_view luaState, const sol::table& left, const sol::table& right);

[[nodiscard]] std::vector<std::string> splitPath(const std::string& path);

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
