#include "beez/plugin/lua/api/sys/tmp_dir.hpp"

#include "beez/core/util/temp_directory.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindTmpDir(sol::table& sysTable)
{
    sysTable["tmp_dir"] = []() -> std::string { return core::systemTempDirectory().string(); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
