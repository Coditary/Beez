#include "beez/plugin/lua/api/date/date_table.hpp"

#include "beez/plugin/lua/api/date/epoch.hpp"
#include "beez/plugin/lua/api/date/format.hpp"
#include "beez/plugin/lua/api/date/info.hpp"
#include "beez/plugin/lua/api/date/utc.hpp"
#include "beez/plugin/lua/api/date/utc_offset.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindDate(const std::shared_ptr<sol::state>& luaState)
{
    sol::table dateTable = luaState->create_table();
    bindFormat(dateTable);
    bindInfo(dateTable, luaState);
    bindEpoch(dateTable);
    bindUtc(dateTable);
    bindUtcOffset(dateTable);
    return dateTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
