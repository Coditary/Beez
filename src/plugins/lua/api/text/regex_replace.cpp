#include "beez/plugin/lua/api/text/regex_replace.hpp"

#include <regex>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindRegexReplace(sol::table& textTable)
{
    textTable["regex_replace"] = [](std::string text,
                                    const std::string& pattern,
                                    const std::string& replacement) -> std::string
    {
        try
        {
            const std::regex expression(pattern);
            return std::regex_replace(text, expression, replacement);
        }
        catch (const std::regex_error& error)
        {
            throw std::runtime_error(std::string("beez.text.regex_replace: invalid pattern: ") +
                                     error.what());
        }
    };
}

}  // namespace beez::plugin::lua
