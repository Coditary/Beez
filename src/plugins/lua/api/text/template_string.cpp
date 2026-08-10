#include "beez/plugin/lua/api/text/template_string.hpp"

#include "beez/plugin/lua/api/text/detail/prebyte_backend.hpp"

#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindTemplateString(sol::table& textTable)
{
    textTable["template"] = [](const std::string& templateString,
                               const sol::table& variables) -> std::string
    { return text_detail::renderTemplateString(templateString, variables); };
}

}  // namespace beez::plugin::lua
