#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <string>

namespace beez::plugin::lua::data_detail
{

[[nodiscard]] sol::object rymlNodeToLua(sol::state_view luaState, ryml::ConstNodeRef node);
[[nodiscard]] std::string luaTableToYamlString(const sol::table& table);

}  // namespace beez::plugin::lua::data_detail
