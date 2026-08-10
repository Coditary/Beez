#include "beez/plugin/lua/api/sys/path_separator.hpp"

#include <filesystem>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindPathSeparator(sol::table& sysTable)
{
    sysTable["path_separator"] = []() -> std::string
    { return {std::filesystem::path::preferred_separator}; };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
