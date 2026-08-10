#include "beez/plugin/lua/api/data/diff.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindDiff(sol::table& dataTable)
{
    dataTable["diff"] = [](const sol::table& left, const sol::table& right) -> sol::table
    { return data_detail::diffTables(sol::state_view(left.lua_state()), left, right); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
