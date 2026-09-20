#include "beez/plugin/lua/api/text/join.hpp"

#include "coditary/utils/strings.hpp"

#include <stdexcept>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::vector<std::string> readStringArray(const sol::table& values)
{
    std::vector<std::string> parts;
    for (std::size_t index = 1;; ++index)
    {
        const sol::object entry = values[index];
        if (!entry.valid())
        {
            break;
        }

        if (!entry.is<std::string>())
        {
            throw std::runtime_error("beez.text.join: all array entries must be strings");
        }

        parts.push_back(entry.as<std::string>());
    }

    return parts;
}

}  // namespace

void bindTextJoin(sol::table& textTable)
{
    textTable["join"] = [](const sol::table& values, const std::string& delimiter) -> std::string
    { return coditary::utils::join(readStringArray(values), delimiter); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
