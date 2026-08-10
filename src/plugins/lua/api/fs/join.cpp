#include "beez/plugin/lua/api/fs/join.hpp"

#include "beez/plugin/lua/api/detail/path.hpp"

#include <stdexcept>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindJoin(sol::table& fsTable)
{
    fsTable["join"] = [](sol::variadic_args segments) -> std::string
    {
        std::vector<std::string> parts;
        parts.reserve(segments.size());
        for (const sol::stack_proxy& segment : segments)
        {
            if (!segment.is<std::string>())
            {
                throw std::runtime_error("beez.fs.join: all arguments must be strings");
            }

            parts.push_back(segment.as<std::string>());
        }

        return api_detail::joinPathSegments(parts);
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
