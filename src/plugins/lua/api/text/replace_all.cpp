#include "beez/plugin/lua/api/text/replace_all.hpp"

#include "coditary/utils/strings.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindReplaceAll(sol::table& textTable)
{
    textTable["replace_all"] = [](const std::string& text,
                                  const std::string& search,
                                  const std::string& replacement) -> std::string
    {
        try
        {
            return coditary::utils::replace_all(text, search, replacement);
        }
        catch (const std::invalid_argument& error)
        {
            throw std::runtime_error(std::string("beez.text.replace_all: ") + error.what());
        }
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
