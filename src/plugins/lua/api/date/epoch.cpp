#include "beez/plugin/lua/api/date/epoch.hpp"

#include "beez/plugin/lua/api/date/detail/time_point.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindEpoch(sol::table& dateTable)
{
    dateTable["epoch"] = []() -> lua_Number { return date_detail::currentEpochSeconds(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
