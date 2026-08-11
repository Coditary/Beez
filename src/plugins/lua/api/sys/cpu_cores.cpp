#include "beez/plugin/lua/api/sys/cpu_cores.hpp"

#include "beez/plugin/lua/api/sys/detail/cpu.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindCpuCores(sol::table& sysTable)
{
    sysTable["cpu_cores"] = []() -> int { return static_cast<int>(sys_detail::cpuCoreCount()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
