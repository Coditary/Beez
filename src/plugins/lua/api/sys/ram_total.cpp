#include "beez/plugin/lua/api/sys/ram_total.hpp"

#include "beez/plugin/lua/api/sys/detail/ram.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindRamTotal(sol::table& sysTable)
{
    sysTable["ram_total"] = []() -> lua_Number
    { return static_cast<lua_Number>(sys_detail::ramTotalBytes()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
