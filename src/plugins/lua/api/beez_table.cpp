#include "beez/plugin/lua/api/beez_table.hpp"

#include "beez/plugin/lua/api/crypto/crypto_table.hpp"
#include "beez/plugin/lua/api/env/env.hpp"
#include "beez/plugin/lua/api/fs/fs_table.hpp"
#include "beez/plugin/lua/api/sys/sys_table.hpp"
#include "beez/plugin/lua/settings/settings_overlay.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void registerBeezApi(const std::shared_ptr<sol::state>& luaState,
                     const core::Context& context,
                     core::BeezSettings& buildSettings)
{
    sol::table beezTable = luaState->create_table();
    bindEnvToTable(beezTable, context);
    beezTable["config"] = [&buildSettings, &context](const sol::object& options)
    {
        if (!options.is<sol::table>())
        {
            throw std::runtime_error("beez.config argument must be a table");
        }

        mergeSettingsFromLuaTable(options.as<sol::table>(), buildSettings);
        buildSettings.applyEnvironment(context);
    };
    beezTable["fs"] = bindFs(luaState, context);
    beezTable["crypto"] = bindCrypto(luaState, context);
    beezTable["sys"] = bindSys(luaState, context);
    (*luaState)["beez"] = beezTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
