#include "beez/plugin/lua/api/text/ends_with.hpp"

#include <string>
#include <string_view>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindEndsWith(sol::table& textTable)
{
    textTable["ends_with"] = [](const std::string& text, const std::string& suffix) -> bool
    {
        const std::string_view TextView = text;
        const std::string_view SuffixView = suffix;
        return TextView.size() >= SuffixView.size()
               && TextView.substr(TextView.size() - SuffixView.size()) == SuffixView;
    };
}

}  // namespace beez::plugin::lua
