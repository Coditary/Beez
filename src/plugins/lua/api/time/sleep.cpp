#include "beez/plugin/lua/api/time/sleep.hpp"

#include "beez/plugin/lua/api/time/detail/clock.hpp"

#include <cstdint>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSleep(sol::table& timeTable)
{
    timeTable["sleep"] = [](const int milliseconds) -> void
    { time_detail::sleepMillis(static_cast<std::int64_t>(milliseconds)); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
