#include "beez/plugin/lua/api/fs/remove.hpp"

#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindRemove(sol::table& fsTable, const core::Context& context)
{
    fsTable["remove"] = [&context](const std::string& path) -> bool
    { return std::filesystem::remove_all(fs_detail::resolvedPath(context, path)) > 0; };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
