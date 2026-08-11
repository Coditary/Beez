#include "beez/plugin/lua/api/text/template_string.hpp"

#include "beez/plugin/lua/api/text/detail/prebyte_backend.hpp"

#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindTemplateString(sol::table& textTable)
{
    textTable["template"] = [](const std::string& templateString,
                               const sol::table& variables) -> std::string
    { return text_detail::renderTemplateString(templateString, variables); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
