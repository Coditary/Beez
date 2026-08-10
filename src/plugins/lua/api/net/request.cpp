#include "beez/plugin/lua/api/net/request.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindRequest(sol::table& netTable, const std::shared_ptr<sol::state>& luaState)
{
    netTable["request"] = [luaState](const std::string& method,
                                     const std::string& url,
                                     const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        if (method.empty())
        {
            throw std::invalid_argument("request method must not be empty");
        }

        return net_detail::responseToTable(
            luaState,
            net_detail::performOrThrow(net_detail::HttpClient::instance().perform(
                net_detail::parseRequestOptions(method, url, optionsTable))));
    };
}

}  // namespace beez::plugin::lua
