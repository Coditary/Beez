#include "beez/plugin/lua/api/data/clone.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindClone(sol::table& dataTable)
{
    dataTable["clone"] = [](const sol::table& table) -> sol::table
    { return data_detail::cloneTable(sol::state_view(table.lua_state()), table); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
