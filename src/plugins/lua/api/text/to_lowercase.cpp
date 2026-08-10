#include "beez/plugin/lua/api/text/to_lowercase.hpp"

#include <algorithm>
#include <cctype>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindToLowercase(sol::table& textTable)
{
    textTable["to_lowercase"] = [](std::string text) -> std::string
    {
        std::ranges::transform(text, text.begin(), [](const unsigned char character) -> char
                               { return static_cast<char>(std::tolower(character)); });
        return text;
    };
}

}  // namespace beez::plugin::lua
