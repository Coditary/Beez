#include "beez/plugin/lua/api/net/get.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/headers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindGet(sol::table& netTable, const std::shared_ptr<sol::state>& luaState)
{
    netTable["get"] =
        [luaState](const std::string& url, const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "GET";
        options.url = url;
        options.headers = headersTable.has_value() ? net_detail::parseHeadersTable(headersTable.value())
                                                   : net_detail::HeaderList {};
        return net_detail::responseToTable(
            luaState,
            net_detail::performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };
}

}  // namespace beez::plugin::lua
