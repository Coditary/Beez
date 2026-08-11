#include "beez/plugin/lua/api/time/now.hpp"

#include "beez/plugin/lua/api/time/detail/clock.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindNow(sol::table& timeTable)
{
    timeTable["now"] = []() -> std::string { return time_detail::nowMillisString(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
