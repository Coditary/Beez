#include "beez/plugin/lua/api/net/ping.hpp"

#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindPing(sol::table& netTable, const std::shared_ptr<sol::state>& luaState)
{
    netTable["ping"] = [luaState](const std::string& url,
                                  const sol::optional<long>& timeoutSeconds) -> sol::table
    {
        const net_detail::PingResult result =
            net_detail::HttpClient::instance().ping(url, timeoutSeconds.value_or(10));
        sol::table response = luaState->create_table();
        response["ok"] = result.reachable;
        response["status"] = result.statusCode;
        response["ms"] = result.milliseconds;
        if (!result.error.empty())
        {
            response["error"] = result.error;
        }
        return response;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
