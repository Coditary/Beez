#include "beez/plugin/lua/api/data/data_table.hpp"

#include "beez/plugin/lua/api/data/clone.hpp"
#include "beez/plugin/lua/api/data/deserialize_file.hpp"
#include "beez/plugin/lua/api/data/deserialize_string.hpp"
#include "beez/plugin/lua/api/data/diff.hpp"
#include "beez/plugin/lua/api/data/get.hpp"
#include "beez/plugin/lua/api/data/merge.hpp"
#include "beez/plugin/lua/api/data/serialize_file.hpp"
#include "beez/plugin/lua/api/data/serialize_string.hpp"
#include "beez/plugin/lua/api/data/set.hpp"
#include "beez/plugin/lua/api/data/validate.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindData(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table dataTable = luaState->create_table();
    bindDeserializeFile(dataTable, luaState, context);
    bindSerializeFile(dataTable, context);
    bindDeserializeString(dataTable, luaState);
    bindSerializeString(dataTable);
    bindMerge(dataTable);
    bindClone(dataTable);
    bindGet(dataTable);
    bindSet(dataTable);
    bindDiff(dataTable);
    bindValidate(dataTable, luaState);
    return dataTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
