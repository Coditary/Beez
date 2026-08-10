#include "beez/plugin/lua/api/text/contains.hpp"

#include <string>
#include <string_view>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindContains(sol::table& textTable)
{
    textTable["contains"] = [](const std::string& text, const std::string& needle) -> bool
    { return std::string_view(text).find(needle) != std::string_view::npos; };
}

}  // namespace beez::plugin::lua
