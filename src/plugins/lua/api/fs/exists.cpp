#include "beez/plugin/lua/api/fs/exists.hpp"

#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindExists(sol::table& fsTable, const core::Context& context)
{
    fsTable["exists"] = [&context](const std::string& path) -> bool
    { return std::filesystem::exists(fs_detail::resolvedPath(context, path)); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
