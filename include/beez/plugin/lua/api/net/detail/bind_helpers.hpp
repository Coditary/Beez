#pragma once

#include "beez/plugin/lua/api/net/detail/http_client.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

#include <memory>
#include <string>
#include <unordered_map>

namespace beez::plugin::lua::net_detail
{

[[nodiscard]] std::string toLower(std::string value);

[[nodiscard]] sol::table headersToTable(const std::shared_ptr<sol::state>& luaState,
                                        const std::unordered_map<std::string, std::string>& headers);

[[nodiscard]] sol::table responseToTable(const std::shared_ptr<sol::state>& luaState,
                                         const HttpResponse& response);

[[nodiscard]] DownloadOptions parseDownloadOptions(const sol::optional<sol::table>& optionsTable);

[[nodiscard]] RequestOptions parseRequestOptions(const std::string& method,
                                                 const std::string& url,
                                                 const sol::optional<sol::table>& optionsTable);

[[nodiscard]] HttpResponse performOrThrow(HttpResponse response);

}  // namespace beez::plugin::lua::net_detail
