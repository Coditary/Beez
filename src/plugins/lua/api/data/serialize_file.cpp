#include "beez/plugin/lua/api/data/serialize_file.hpp"

#include "beez/plugin/lua/api/data/detail/codec.hpp"
#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSerializeFile(sol::table& dataTable, const core::Context& context)
{
    dataTable["serialize_file"] =
        [&context](const std::string& path, const sol::table& table, const sol::object& options)
    {
        const std::filesystem::path Resolved = fs_detail::resolvedPath(context, path);
        const data_detail::DataFormat Format = data_detail::resolveFormat(Resolved, options);
        data_detail::serializeFile(Resolved, table, Format);
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
