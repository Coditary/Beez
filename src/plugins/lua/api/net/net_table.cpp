#include "beez/plugin/lua/api/net/net_table.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"
#include "beez/plugin/lua/api/net/detail/headers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

[[nodiscard]] sol::table headersToTable(const std::shared_ptr<sol::state>& luaState,
                                        const std::unordered_map<std::string, std::string>& headers)
{
    sol::table headerTable = luaState->create_table();
    for (const auto& [name, value] : headers)
    {
        headerTable[name] = value;
    }
    return headerTable;
}

[[nodiscard]] sol::table responseToTable(const std::shared_ptr<sol::state>& luaState,
                                         const net_detail::HttpResponse& response)
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

[[nodiscard]] net_detail::DownloadOptions
parseDownloadOptions(const sol::optional<sol::table>& optionsTable)
{
    net_detail::DownloadOptions options;
    if (!optionsTable.has_value())
    {
        return options;
    }

    const sol::table Table = optionsTable.value();
    if (const sol::object HeadersValue = Table["headers"]; HeadersValue.valid())
    {
        options.headers = net_detail::parseHeadersObject(HeadersValue);
    }
    if (const sol::object TimeoutValue = Table["timeout"]; TimeoutValue.valid())
    {
        options.timeoutSeconds = TimeoutValue.as<long>();
    }
    if (const sol::object FollowValue = Table["follow_redirects"]; FollowValue.valid())
    {
        options.followRedirects = FollowValue.as<bool>();
    }

    return options;
}

[[nodiscard]] net_detail::RequestOptions
parseRequestOptions(const std::string& method,
                    const std::string& url,
                    const sol::optional<sol::table>& optionsTable)
{
    net_detail::RequestOptions options;
    options.method = method;
    options.url = url;
    if (!optionsTable.has_value())
    {
        return options;
    }

    const sol::table Table = optionsTable.value();
    if (const sol::object BodyValue = Table["body"]; BodyValue.valid())
    {
        if (!BodyValue.is<std::string>())
        {
            throw std::runtime_error("request option 'body' must be a string");
        }
        options.body = BodyValue.as<std::string>();
    }
    if (const sol::object HeadersValue = Table["headers"]; HeadersValue.valid())
    {
        options.headers = net_detail::parseHeadersObject(HeadersValue);
    }
    if (const sol::object TimeoutValue = Table["timeout"]; TimeoutValue.valid())
    {
        options.timeoutSeconds = TimeoutValue.as<long>();
    }
    if (const sol::object FollowValue = Table["follow_redirects"]; FollowValue.valid())
    {
        options.followRedirects = FollowValue.as<bool>();
    }

    return options;
}

[[nodiscard]] net_detail::HttpResponse
performOrThrow(net_detail::HttpResponse response)
{
    if (!response.error.empty())
    {
        throw std::runtime_error("request failed: " + response.error);
    }

    return response;
}

}  // namespace

sol::table bindNet(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table netTable = luaState->create_table();

    netTable["get"] =
        [luaState](const std::string& url, const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "GET";
        options.url = url;
        options.headers = headersTable.has_value()
                              ? net_detail::parseHeadersTable(headersTable.value())
                              : net_detail::HeaderList {};
        return responseToTable(luaState,
                               performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };

    netTable["post"] =
        [luaState](const std::string& url,
                   const std::string& body,
                   const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "POST";
        options.url = url;
        options.body = body;
        options.headers = headersTable.has_value()
                              ? net_detail::parseHeadersTable(headersTable.value())
                              : net_detail::HeaderList {};
        return responseToTable(luaState,
                               performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };

    netTable["put"] =
        [luaState](const std::string& url,
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
        return responseToTable(luaState,
                               performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };

    netTable["delete"] =
        [luaState](const std::string& url, const sol::optional<sol::table>& headersTable) -> sol::table
    {
        net_detail::RequestOptions options;
        options.method = "DELETE";
        options.url = url;
        options.headers = headersTable.has_value()
                              ? net_detail::parseHeadersTable(headersTable.value())
                              : net_detail::HeaderList {};
        return responseToTable(luaState,
                               performOrThrow(net_detail::HttpClient::instance().perform(options)));
    };

    netTable["request"] =
        [luaState](const std::string& method,
                   const std::string& url,
                   const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        if (method.empty())
        {
            throw std::invalid_argument("request method must not be empty");
        }

        return responseToTable(
            luaState,
            performOrThrow(net_detail::HttpClient::instance().perform(
                parseRequestOptions(method, url, optionsTable))));
    };

    netTable["upload"] =
        [&context, luaState](const std::string& url,
                             const std::string& filePath,
                             const sol::optional<sol::table>& headersTable) -> sol::table
    {
        const std::filesystem::path Resolved =
            api_detail::resolvePath(context.projectRoot(), filePath);
        const net_detail::HeaderList Headers =
            headersTable.has_value() ? net_detail::parseHeadersTable(headersTable.value())
                                     : net_detail::HeaderList {};
        return responseToTable(
            luaState,
            performOrThrow(
                net_detail::HttpClient::instance().uploadFile(url, Resolved, Headers)));
    };

    netTable["download"] =
        [luaState, &context](const std::string& url,
                   const std::string& destinationPath,
                   const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path Destination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        const net_detail::DownloadOptions Options = parseDownloadOptions(optionsTable);
        const std::uintmax_t Bytes = net_detail::HttpClient::instance().download(
            url, Destination, Options);

        sol::table result = luaState->create_table();
        result["path"] = Destination.generic_string();
        result["bytes"] = static_cast<std::uint64_t>(Bytes);
        return result;
    };

    netTable["download_and_verify"] =
        [luaState, &context](const std::string& url,
                   const std::string& destinationPath,
                   const std::string& algorithm,
                   const std::string& expectedHash,
                   const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path Destination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        const net_detail::DownloadOptions Options = parseDownloadOptions(optionsTable);
        const std::uintmax_t Bytes = net_detail::HttpClient::instance().download(
            url, Destination, Options);

        const std::string ActualHash = crypto_detail::hashFile(Destination, algorithm);
        const std::string Expected = toLower(expectedHash);
        const std::string Actual = toLower(ActualHash);
        if (Actual != Expected)
        {
            throw std::runtime_error("hash mismatch for downloaded file: expected " + Expected +
                                     ", got " + Actual);
        }

        sol::table result = luaState->create_table();
        result["path"] = Destination.generic_string();
        result["bytes"] = static_cast<std::uint64_t>(Bytes);
        result["hash"] = ActualHash;
        result["verified"] = true;
        return result;
    };

    netTable["ping"] =
        [luaState](const std::string& url, const sol::optional<long>& timeoutSeconds) -> sol::table
    {
        const net_detail::PingResult Result = net_detail::HttpClient::instance().ping(
            url, timeoutSeconds.value_or(10));
        sol::table result = luaState->create_table();
        result["ok"] = Result.reachable;
        result["status"] = Result.statusCode;
        result["ms"] = Result.milliseconds;
        if (!Result.error.empty())
        {
            result["error"] = Result.error;
        }
        return result;
    };

    netTable["is_online"] =
        [](const sol::optional<long>& timeoutSeconds) -> bool
    {
        return net_detail::HttpClient::instance().isOnline(timeoutSeconds.value_or(3));
    };

    netTable["set_proxy"] =
        [](const sol::object& proxyValue)
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

    return netTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
