#include "beez/plugin/lua/api/time/uptime.hpp"

#include "beez/plugin/lua/api/time/detail/clock.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindUptime(sol::table& timeTable)
{
    timeTable["uptime"] = []() -> std::string { return time_detail::uptimeMillisString(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
