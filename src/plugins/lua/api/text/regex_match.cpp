#include "beez/plugin/lua/api/text/regex_match.hpp"

#include <regex>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindRegexMatch(sol::table& textTable)
{
    textTable["regex_match"] = [](const std::string& text, const std::string& pattern) -> bool
    {
        try
        {
            const std::regex expression(pattern);
            return std::regex_match(text, expression);
        }
        catch (const std::regex_error& error)
        {
            throw std::runtime_error(std::string("beez.text.regex_match: invalid pattern: ") + error.what());
        }
    };
}

}  // namespace beez::plugin::lua
