#include "beez/plugin/lua/api/fs/copy.hpp"

#include "beez/plugin/lua/api/fs/detail/operations.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindCopy(sol::table& fsTable, const core::Context& context)
{
    fsTable["copy"] =
        [&context](const std::string& sourcePath,
                   const std::string& destinationPath,
                   sol::optional<bool> overwrite) -> void
    {
        fs_detail::copyPath(context, sourcePath, destinationPath, overwrite.value_or(false));
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
