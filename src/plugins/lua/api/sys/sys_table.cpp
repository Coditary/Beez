#include "beez/plugin/lua/api/sys/sys_table.hpp"

#include "beez/plugin/lua/api/sys/cpu_cores.hpp"
#include "beez/plugin/lua/api/sys/cpu_threads.hpp"
#include "beez/plugin/lua/api/sys/cwd.hpp"
#include "beez/plugin/lua/api/sys/is_tty.hpp"
#include "beez/plugin/lua/api/sys/path_separator.hpp"
#include "beez/plugin/lua/api/sys/pid.hpp"
#include "beez/plugin/lua/api/sys/ram_free.hpp"
#include "beez/plugin/lua/api/sys/ram_total.hpp"
#include "beez/plugin/lua/api/sys/tmp_dir.hpp"
#include "beez/plugin/lua/api/sys/user.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindSys(const std::shared_ptr<sol::state>& luaState, const core::Context& /*context*/)
{
    sol::table sysTable = luaState->create_table();
    bindCpuCores(sysTable);
    bindCpuThreads(sysTable);
    bindRamTotal(sysTable);
    bindRamFree(sysTable);
    bindIsTty(sysTable);
    bindCwd(sysTable);
    bindTmpDir(sysTable);
    bindPathSeparator(sysTable);
    bindPid(sysTable);
    bindUser(sysTable);
    return sysTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
