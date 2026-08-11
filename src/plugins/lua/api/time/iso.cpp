#include "beez/plugin/lua/api/time/iso.hpp"

#include "beez/plugin/lua/api/time/detail/clock.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIso(sol::table& timeTable)
{
    timeTable["iso"] = []() -> std::string { return time_detail::iso8601UtcNow(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
