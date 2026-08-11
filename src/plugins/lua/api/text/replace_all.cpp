#include "beez/plugin/lua/api/text/replace_all.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindReplaceAll(sol::table& textTable)
{
    textTable["replace_all"] = [](std::string text,
                                  const std::string& search,
                                  const std::string& replacement) -> std::string
    {
        if (search.empty())
        {
            throw std::runtime_error("beez.text.replace_all: search string must not be empty");
        }

        std::size_t position = 0;
        while ((position = text.find(search, position)) != std::string::npos)
        {
            text.replace(position, search.size(), replacement);
            position += replacement.size();
        }

        return text;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
