#include "beez/plugin/lua/runtime/lua_table_util.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table cloneLuaTable(const std::shared_ptr<sol::state>& luaState, const sol::table& source)
{
    return data_detail::cloneTable(sol::state_view(*luaState), source);
}

void deepMergeLuaTables(sol::table& target, const sol::table& source)
{
    data_detail::deepMerge(target, source);
}

sol::table deepMergeLuaTablesCopy(const std::shared_ptr<sol::state>& luaState,
                                  const sol::table& base,
                                  const sol::table& overlay)
{
    sol::table merged = cloneLuaTable(luaState, base);
    deepMergeLuaTables(merged, overlay);
    return merged;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
