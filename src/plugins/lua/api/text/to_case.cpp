#include "beez/plugin/lua/api/text/to_case.hpp"

#include <cctype>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindToCase(sol::table& textTable)
{
    textTable["to_case"] = [](std::string text) -> std::string
    {
        bool capitalizeNext = true;
        for (char& character : text)
        {
            const unsigned char unsignedCharacter = static_cast<unsigned char>(character);
            if (std::isspace(unsignedCharacter) != 0)
            {
                capitalizeNext = true;
                continue;
            }

            if (capitalizeNext)
            {
                character = static_cast<char>(std::toupper(unsignedCharacter));
                capitalizeNext = false;
            }
            else
            {
                character = static_cast<char>(std::tolower(unsignedCharacter));
            }
        }

        return text;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
