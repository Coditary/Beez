#include "beez/plugin/lua/api/data/merge.hpp"

#include "beez/plugin/lua/api/data/table_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindMerge(sol::table& dataTable)
{
    dataTable["merge"] = [](sol::table target, const sol::table& source) -> sol::table
    {
        data_detail::deepMerge(target, source);
        return target;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
