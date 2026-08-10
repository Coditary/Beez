#include "beez/plugin/lua/api/sys/ram_free.hpp"

#include "beez/plugin/lua/api/sys/detail/ram.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindRamFree(sol::table& sysTable)
{
    sysTable["ram_free"] = []() -> lua_Number
    { return static_cast<lua_Number>(sys_detail::ramFreeBytes()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
