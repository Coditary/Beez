#include "beez/plugin/lua/api/sys/cwd.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindCwd(sol::table& sysTable)
{
    sysTable["cwd"] = []() -> std::string
    { return std::filesystem::current_path().string(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
