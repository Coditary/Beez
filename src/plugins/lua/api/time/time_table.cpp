#include "beez/plugin/lua/api/time/time_table.hpp"

#include "beez/plugin/lua/api/time/iso.hpp"
#include "beez/plugin/lua/api/time/now.hpp"
#include "beez/plugin/lua/api/time/sleep.hpp"
#include "beez/plugin/lua/api/time/sleep_s.hpp"
#include "beez/plugin/lua/api/time/uptime.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindTime(const std::shared_ptr<sol::state>& luaState)
{
    sol::table timeTable = luaState->create_table();
    bindNow(timeTable);
    bindUptime(timeTable);
    bindIso(timeTable);
    bindSleep(timeTable);
    bindSleepS(timeTable);
    return timeTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
