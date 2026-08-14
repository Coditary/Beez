#include "beez/plugin/lua/api/shell/shell_table.hpp"

#include "beez/plugin/lua/api/shell/run.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindShell(const std::shared_ptr<sol::state>& luaState, sol::table& beezTable)
{
    sol::table shellTable = luaState->create_table();
    shellTable["run"] = &runShellCommand;
    beezTable["shell"] = shellTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
