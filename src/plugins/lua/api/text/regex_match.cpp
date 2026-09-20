#include "beez/plugin/lua/api/text/regex_match.hpp"

#include "coditary/utils/strings.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindRegexMatch(sol::table& textTable)
{
    textTable["regex_match"] = [](const std::string& text, const std::string& pattern) -> bool
    {
        try
        {
            return coditary::utils::regex_match(text, pattern);
        }
        catch (const std::invalid_argument& error)
        {
            throw std::runtime_error(std::string("beez.text.regex_match: ") + error.what());
        }
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
