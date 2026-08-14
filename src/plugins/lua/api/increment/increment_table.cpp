#include "beez/plugin/lua/api/increment/increment_table.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIncrement(const std::shared_ptr<sol::state>& luaState, sol::table& beezTable)
{
    const auto IncrementScript =
        std::filesystem::path(BEEZ_LUA_API_SOURCE_DIR) / "increment" / "increment.lua";
    if (!std::filesystem::exists(IncrementScript))
    {
        throw std::runtime_error("beez increment runtime missing: " + IncrementScript.string());
    }

    sol::object loaded = luaState->script_file(IncrementScript.string());
    if (!loaded.is<sol::table>())
    {
        throw std::runtime_error("beez increment runtime must return a table");
    }

    beezTable["increment"] = loaded.as<sol::table>();
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
