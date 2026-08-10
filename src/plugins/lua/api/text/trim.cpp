#include "beez/plugin/lua/api/text/trim.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindTrim(sol::table& textTable)
{
    textTable["trim"] = [](const std::string& text) -> std::string
    {
        const std::string_view TextView = text;
        const auto isNotSpace = [](const unsigned char character) -> bool
        { return std::isspace(character) == 0; };

        const auto begin = std::ranges::find_if(TextView, isNotSpace);
        const auto end = std::ranges::find_if(std::views::reverse(TextView), isNotSpace).base();
        if (begin >= end)
        {
            return {};
        }

        return std::string(begin, end);
    };
}

}  // namespace beez::plugin::lua
