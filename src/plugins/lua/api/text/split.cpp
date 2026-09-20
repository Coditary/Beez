#include "beez/plugin/lua/api/text/split.hpp"

#include "coditary/utils/strings.hpp"

#include <stdexcept>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSplit(sol::table& textTable, const std::shared_ptr<sol::state>& luaState)
{
    textTable["split"] = [luaState](const std::string& text,
                                    const std::string& delimiter) -> sol::table
    {
        try
        {
            const std::vector<std::string> parts = coditary::utils::split(text, delimiter);
            sol::table result = luaState->create_table(static_cast<int>(parts.size()), 0);
            for (std::size_t index = 0; index < parts.size(); ++index)
            {
                result[index + 1U] = parts[index];
            }

            return result;
        }
        catch (const std::invalid_argument& error)
        {
            throw std::runtime_error(std::string("beez.text.split: ") + error.what());
        }
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
