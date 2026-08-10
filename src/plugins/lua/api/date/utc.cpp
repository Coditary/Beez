#include "beez/plugin/lua/api/date/utc.hpp"

#include "beez/plugin/lua/api/date/detail/time_point.hpp"

#include <ctime>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindUtc(sol::table& dateTable)
{
    dateTable["utc"] = sol::overload(
        []() -> std::string { return date_detail::utcIso8601(std::time(nullptr)); },
        [](const double epoch) -> std::string
        { return date_detail::utcIso8601(static_cast<std::time_t>(epoch)); },
        [](const double epoch, const int offsetMinutes) -> std::string
        {
            return date_detail::utcIso8601WithOffset(static_cast<std::time_t>(epoch),
                                                     offsetMinutes);
        });
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
