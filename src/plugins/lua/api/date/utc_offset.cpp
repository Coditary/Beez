#include "beez/plugin/lua/api/date/utc_offset.hpp"

#include "beez/plugin/lua/api/date/detail/time_point.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindUtcOffset(sol::table& dateTable)
{
    dateTable["utc_offset"] = []() -> int { return date_detail::localUtcOffsetMinutes(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
