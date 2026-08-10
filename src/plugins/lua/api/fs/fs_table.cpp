#include "beez/plugin/lua/api/fs/fs_table.hpp"

#include "beez/plugin/lua/api/fs/copy.hpp"
#include "beez/plugin/lua/api/fs/exists.hpp"
#include "beez/plugin/lua/api/fs/glob.hpp"
#include "beez/plugin/lua/api/fs/join.hpp"
#include "beez/plugin/lua/api/fs/remove.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindFs(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table fsTable = luaState->create_table();
    bindGlob(fsTable, luaState, context);
    bindExists(fsTable, context);
    bindCopy(fsTable, context);
    bindRemove(fsTable, context);
    bindJoin(fsTable);
    return fsTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
