#include "beez/plugin/lua/api/net/is_online.hpp"

#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindIsOnline(sol::table& netTable)
{
    netTable["is_online"] =
        [](const sol::optional<long>& timeoutSeconds) -> bool
    { return net_detail::HttpClient::instance().isOnline(timeoutSeconds.value_or(3)); };
}

}  // namespace beez::plugin::lua
