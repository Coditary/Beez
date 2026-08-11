#include "beez/plugin/lua/api/sys/pid.hpp"

#include <unistd.h>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindPid(sol::table& sysTable)
{
    sysTable["pid"] = []() -> int { return static_cast<int>(getpid()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
