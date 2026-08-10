#include "beez/plugin/lua/api/text/to_uppercase.hpp"

#include <algorithm>
#include <cctype>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindToUppercase(sol::table& textTable)
{
    textTable["to_uppercase"] = [](std::string text) -> std::string
    {
        std::ranges::transform(text,
                               text.begin(),
                               [](const unsigned char character) -> char
                               { return static_cast<char>(std::toupper(character)); });
        return text;
    };
}

}  // namespace beez::plugin::lua
