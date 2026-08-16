#include "beez/plugin/lua/api/increment/increment_table.hpp"

#include "beez_increment_runtime.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIncrement(const std::shared_ptr<sol::state>& luaState, sol::table& beezTable)
{
    const sol::object loaded = luaState->script(kIncrementRuntimeSource, "beez_increment");
    if (!loaded.is<sol::table>())
    {
        throw std::runtime_error("beez increment runtime must return a table");
    }

    beezTable["increment"] = loaded.as<sol::table>();
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
