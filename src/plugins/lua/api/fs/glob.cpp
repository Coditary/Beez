#include "beez/plugin/lua/api/fs/glob.hpp"

#include "beez/plugin/lua/api/fs/detail/glob.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindGlob(sol::table& fsTable,
              const std::shared_ptr<sol::state>& luaState,
              const core::Context& context)
{
    fsTable["glob"] = [luaState, &context](const std::string& pattern) -> sol::table
    {
        return fs_detail::globPatternsToTable(luaState, {pattern}, context.projectRoot());
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
