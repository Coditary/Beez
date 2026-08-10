#include "beez/plugin/lua/api/net/download.hpp"

#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindDownload(sol::table& netTable,
                  const std::shared_ptr<sol::state>& luaState,
                  const core::Context& context)
{
    netTable["download"] =
        [luaState, &context](const std::string& url,
                             const std::string& destinationPath,
                             const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path destination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        const net_detail::DownloadOptions options = net_detail::parseDownloadOptions(optionsTable);
        const std::uintmax_t bytes = net_detail::HttpClient::instance().download(url, destination, options);

        sol::table result = luaState->create_table();
        result["path"] = destination.generic_string();
        result["bytes"] = static_cast<std::uint64_t>(bytes);
        return result;
    };
}

}  // namespace beez::plugin::lua
