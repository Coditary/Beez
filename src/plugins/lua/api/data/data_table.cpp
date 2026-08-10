#include "beez/plugin/lua/api/data/data_table.hpp"

#include "beez/plugin/lua/api/data/deserialize_file.hpp"

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
    bindClone(dataTable, luaState);
    bindGet(dataTable);
    bindSet(dataTable);
    bindDiff(dataTable, luaState);
    bindValidate(dataTable, luaState);
    return dataTable;
}

}  // namespace beez::plugin::lua
