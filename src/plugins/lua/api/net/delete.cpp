#include "beez/plugin/lua/api/net/delete.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/headers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindDelete(sol::table& netTable, const std::shared_ptr<sol::state>& luaState)
{
    netTable["delete"] =
        [luaState](const std::string& url, const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "DELETE";
        options.url = url;
        options.headers = headersTable.has_value() ? net_detail::parseHeadersTable(headersTable.value())
                                                   : net_detail::HeaderList {};
        return net_detail::responseToTable(
            luaState,
            net_detail::performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };
}

}  // namespace beez::plugin::lua
