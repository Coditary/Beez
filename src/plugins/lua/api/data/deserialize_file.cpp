#include "beez/plugin/lua/api/data/deserialize_file.hpp"

#include "beez/plugin/lua/api/data/detail/codec.hpp"
#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include <filesystem>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindDeserializeFile(sol::table& dataTable,
                         const std::shared_ptr<sol::state>& luaState,
                         const core::Context& context)
{
    dataTable["deserialize_file"] = [luaState, &context](const std::string& path,
                                                         const sol::object& options) -> sol::table
    {
        const std::filesystem::path Resolved = fs_detail::resolvedPath(context, path);
        const data_detail::DataFormat Format = data_detail::resolveFormat(Resolved, options);
        return data_detail::deserializeFile(*luaState, Resolved, Format);
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
