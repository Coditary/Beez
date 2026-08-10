#include "beez/plugin/lua/api/net/detail/headers.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua::net_detail
{

HeaderList parseHeadersTable(const sol::table& headersTable)
{
    HeaderList headers;
    headersTable.for_each(
        [&headers](const sol::object& key, const sol::object& value)
        {
            if (!key.is<std::string>())
            {
                throw std::runtime_error("header keys must be strings");
            }

            if (!value.is<std::string>())
            {
                throw std::runtime_error("header values must be strings");
            }

            headers.emplace_back(key.as<std::string>(), value.as<std::string>());
        });
    return headers;
}

HeaderList parseHeadersObject(const sol::object& headersValue)
{
    if (!headersValue.valid() || headersValue.is<sol::lua_nil_t>())
    {
        return {};
    }

    if (!headersValue.is<sol::table>())
    {
        throw std::runtime_error("headers must be a table of string keys and string values");
    }

    return parseHeadersTable(headersValue.as<sol::table>());
}

}  // namespace beez::plugin::lua::net_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
