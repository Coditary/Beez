#include "beez/plugin/lua/api/text/split.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::vector<std::string> splitText(std::string_view text, std::string_view delimiter)
{
    if (delimiter.empty())
    {
        throw std::runtime_error("beez.text.split: delimiter must not be empty");
    }

    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t position = text.find(delimiter, start);
        if (position == std::string_view::npos)
        {
            parts.emplace_back(text.substr(start));
            break;
        }

        parts.emplace_back(text.substr(start, position - start));
        start = position + delimiter.size();
    }

    return parts;
}

}  // namespace

void bindSplit(sol::table& textTable, const std::shared_ptr<sol::state>& luaState)
{
    textTable["split"] = [luaState](const std::string& text,
                                    const std::string& delimiter) -> sol::table
    {
        const std::vector<std::string> parts = splitText(text, delimiter);
        sol::table result = luaState->create_table(static_cast<int>(parts.size()), 0);
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            result[index + 1U] = parts[index];
        }

        return result;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
