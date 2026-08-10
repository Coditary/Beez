#include "beez/plugin/lua/api/net/upload.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/headers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindUpload(sol::table& netTable,
                const std::shared_ptr<sol::state>& luaState,
                const core::Context& context)
{
    netTable["upload"] =
        [&context, luaState](const std::string& url,
                             const std::string& filePath,
                             const sol::optional<sol::table>& headersTable) -> sol::table
    {
        const std::filesystem::path resolved =
            api_detail::resolvePath(context.projectRoot(), filePath);
        const net_detail::HeaderList headers =
            headersTable.has_value() ? net_detail::parseHeadersTable(headersTable.value())
                                     : net_detail::HeaderList {};
        return net_detail::responseToTable(
            luaState,
            net_detail::performOrThrow(
                net_detail::HttpClient::instance().uploadFile(url, resolved, headers)));
    };
}

}  // namespace beez::plugin::lua
