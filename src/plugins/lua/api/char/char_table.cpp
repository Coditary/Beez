#include "beez/plugin/lua/api/char/char_table.hpp"

#include "beez/plugin/lua/api/char/quote.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindChar(const std::shared_ptr<sol::state>& luaState, sol::table& beezTable)
{
    sol::table charTable = luaState->create_table();
    charTable["quote"] = [](const std::string& value) { return shellQuote(value); };
    beezTable["char"] = charTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
