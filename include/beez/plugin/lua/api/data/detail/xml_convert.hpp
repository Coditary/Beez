#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

#include <string>

namespace beez::plugin::lua::data_detail
{

[[nodiscard]] sol::table xmlStringToLua(sol::state_view luaState, const std::string& content);
[[nodiscard]] std::string luaTableToXmlString(const sol::table& table);

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
