#include "beez/plugin/lua/api/text/starts_with.hpp"

#include <string>
#include <string_view>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindStartsWith(sol::table& textTable)
{
    textTable["starts_with"] = [](const std::string& text, const std::string& prefix) -> bool
    {
        const std::string_view TextView = text;
        const std::string_view PrefixView = prefix;
        return TextView.size() >= PrefixView.size() &&
               TextView.substr(0, PrefixView.size()) == PrefixView;
    };
}

}  // namespace beez::plugin::lua
