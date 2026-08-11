#include "beez/plugin/lua/api/data/set.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSet(sol::table& dataTable)
{
    dataTable["set"] =
        [](sol::table table, const std::string& path, const sol::object& value) -> sol::table
    {
        data_detail::setPath(table, path, value);
        return table;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
