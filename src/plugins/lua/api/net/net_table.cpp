#include "beez/plugin/lua/api/net/net_table.hpp"

#include "beez/plugin/lua/api/net/delete.hpp"
#include "beez/plugin/lua/api/net/download.hpp"
#include "beez/plugin/lua/api/net/download_and_verify.hpp"
#include "beez/plugin/lua/api/net/get.hpp"
#include "beez/plugin/lua/api/net/is_online.hpp"
#include "beez/plugin/lua/api/net/ping.hpp"
#include "beez/plugin/lua/api/net/post.hpp"
#include "beez/plugin/lua/api/net/put.hpp"
#include "beez/plugin/lua/api/net/request.hpp"
#include "beez/plugin/lua/api/net/set_proxy.hpp"
#include "beez/plugin/lua/api/net/upload.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindNet(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table netTable = luaState->create_table();
    bindGet(netTable, luaState);
    bindPost(netTable, luaState);
    bindPut(netTable, luaState);
    bindDelete(netTable, luaState);
    bindRequest(netTable, luaState);
    bindUpload(netTable, luaState, context);
    bindDownload(netTable, luaState, context);
    bindDownloadAndVerify(netTable, luaState, context);
    bindPing(netTable, luaState);
    bindIsOnline(netTable);
    bindSetProxy(netTable);
    return netTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
