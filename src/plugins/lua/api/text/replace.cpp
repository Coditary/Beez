#include "beez/plugin/lua/api/text/replace.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindReplace(sol::table& textTable)
{
    textTable["replace"] = [](std::string text, const std::string& search, const std::string& replacement) -> std::string
    {
        if (search.empty())
        {
            throw std::runtime_error("beez.text.replace: search string must not be empty");
        }

        const std::size_t position = text.find(search);
        if (position == std::string::npos)
        {
            return text;
        }

        text.replace(position, search.size(), replacement);
        return text;
    };
}

}  // namespace beez::plugin::lua
