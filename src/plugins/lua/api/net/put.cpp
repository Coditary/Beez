#include "beez/plugin/lua/api/net/put.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/headers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindPut(sol::table& netTable, const std::shared_ptr<sol::state>& luaState)
{
    netTable["put"] = [luaState](const std::string& url,
                                 const std::string& body,
                                 const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "PUT";
        options.url = url;
        options.body = body;
        options.headers = headersTable.has_value()
                              ? net_detail::parseHeadersTable(headersTable.value())
                              : net_detail::HeaderList {};
        return net_detail::responseToTable(
            luaState,
            net_detail::performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
