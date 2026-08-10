#include "beez/plugin/lua/api/data/get.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindGet(sol::table& dataTable)
{
    dataTable["get"] =
        [](const sol::table& table, const std::string& path, const sol::object& defaultValue) -> sol::object
    { return data_detail::getPath(table, path, defaultValue); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
