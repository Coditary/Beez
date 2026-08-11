#include "beez/plugin/lua/api/time/sleep_s.hpp"

#include "beez/plugin/lua/api/time/detail/clock.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSleepS(sol::table& timeTable)
{
    timeTable["sleep_s"] = [](double seconds) -> void { time_detail::sleepSeconds(seconds); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
