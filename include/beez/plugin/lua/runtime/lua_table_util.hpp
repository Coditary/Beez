#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

#include <memory>

namespace beez::plugin::lua
{

[[nodiscard]] sol::table cloneLuaTable(const std::shared_ptr<sol::state>& luaState,
                                       const sol::table& source);

void deepMergeLuaTables(sol::table& target, const sol::table& source);

[[nodiscard]] sol::table deepMergeLuaTablesCopy(const std::shared_ptr<sol::state>& luaState,
                                                  const sol::table& base,
                                                  const sol::table& overlay);

}  // namespace beez::plugin::lua
