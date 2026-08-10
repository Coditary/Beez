#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"

#include "beez/plugin/lua/api/net/detail/headers.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua::net_detail
{

std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });
    return value;
}

sol::table headersToTable(const std::shared_ptr<sol::state>& luaState,
                          const std::unordered_map<std::string, std::string>& headers)
{
    sol::table headerTable = luaState->create_table();
    for (const auto& [name, headerValue] : headers)
    {
        headerTable[name] = headerValue;
    }
    return headerTable;
}

sol::table responseToTable(const std::shared_ptr<sol::state>& luaState,
                           const HttpResponse& response)
{
    sol::table result = luaState->create_table();
    result["status"] = response.statusCode;
    result["body"] = response.body;
    result["ok"] = response.ok();
    result["headers"] = headersToTable(luaState, response.headers);
    if (!response.error.empty())
    {
        result["error"] = response.error;
    }
    return result;
}

DownloadOptions parseDownloadOptions(const sol::optional<sol::table>& optionsTable)
{
    DownloadOptions options;
    if (!optionsTable.has_value())
    {
        return options;
    }

    const sol::table table = optionsTable.value();
    if (const sol::object headersValue = table["headers"]; headersValue.valid())
    {
        options.headers = parseHeadersObject(headersValue);
    }
    if (const sol::object timeoutValue = table["timeout"]; timeoutValue.valid())
    {
        options.timeoutSeconds = timeoutValue.as<long>();
    }
    if (const sol::object followValue = table["follow_redirects"]; followValue.valid())
    {
        options.followRedirects = followValue.as<bool>();
    }

    return options;
}

RequestOptions parseRequestOptions(const std::string& method,
                                   const std::string& url,
                                   const sol::optional<sol::table>& optionsTable)
{
    RequestOptions options;
    options.method = method;
    options.url = url;
    if (!optionsTable.has_value())
    {
        return options;
    }

    const sol::table table = optionsTable.value();
    if (const sol::object bodyValue = table["body"]; bodyValue.valid())
    {
        if (!bodyValue.is<std::string>())
        {
            throw std::runtime_error("request option 'body' must be a string");
        }
        options.body = bodyValue.as<std::string>();
    }
    if (const sol::object headersValue = table["headers"]; headersValue.valid())
    {
        options.headers = parseHeadersObject(headersValue);
    }
    if (const sol::object timeoutValue = table["timeout"]; timeoutValue.valid())
    {
        options.timeoutSeconds = timeoutValue.as<long>();
    }
    if (const sol::object followValue = table["follow_redirects"]; followValue.valid())
    {
        options.followRedirects = followValue.as<bool>();
    }

    return options;
}

HttpResponse performOrThrow(const HttpResponse& response)
{
    if (!response.error.empty())
    {
        throw std::runtime_error("request failed: " + response.error);
    }

    return response;
}

}  // namespace beez::plugin::lua::net_detail
