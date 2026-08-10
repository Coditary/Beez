#include "beez/plugin/lua/api/sys/cpu_threads.hpp"

#include "beez/plugin/lua/api/sys/detail/cpu.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindCpuThreads(sol::table& sysTable)
{
    sysTable["cpu_threads"] = []() -> int
    { return static_cast<int>(sys_detail::cpuThreadCount()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
