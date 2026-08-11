#include "beez/plugin/lua/api/net/set_proxy.hpp"

#include "beez/plugin/lua/api/net/detail/http_client.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSetProxy(sol::table& netTable)
{
    netTable["set_proxy"] = [](const sol::object& proxyValue)
    {
        if (!proxyValue.valid() || proxyValue.is<sol::lua_nil_t>())
        {
            net_detail::HttpClient::instance().clearProxy();
            return;
        }

        if (!proxyValue.is<std::string>())
        {
            throw std::runtime_error("proxy url must be a string or nil");
        }

        net_detail::HttpClient::instance().setProxy(proxyValue.as<std::string>());
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming)
