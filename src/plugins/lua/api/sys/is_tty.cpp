#include "beez/plugin/lua/api/sys/is_tty.hpp"

#include <unistd.h>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIsTty(sol::table& sysTable)
{
    sysTable["is_tty"] = []() -> bool
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise) -- POSIX isatty contract
        return isatty(STDOUT_FILENO) != 0;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
