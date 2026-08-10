#include "beez/plugin/lua/api/net/download_and_verify.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"
#include "beez/plugin/lua/api/net/detail/bind_helpers.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

void bindDownloadAndVerify(sol::table& netTable,
                           const std::shared_ptr<sol::state>& luaState,
                           const core::Context& context)
{
    netTable["download_and_verify"] =
        [luaState, &context](const std::string& url,
                             const std::string& destinationPath,
                             const std::string& algorithm,
                             const std::string& expectedHash,
                             const sol::optional<sol::table>& optionsTable) -> sol::table
    {
        const std::filesystem::path destination =
            api_detail::resolvePath(context.projectRoot(), destinationPath);
        const net_detail::DownloadOptions options = net_detail::parseDownloadOptions(optionsTable);
        const std::uintmax_t bytes =
            net_detail::HttpClient::instance().download(url, destination, options);

        const std::string actualHash = crypto_detail::hashFile(destination, algorithm);
        const std::string expected = net_detail::toLower(expectedHash);
        const std::string actual = net_detail::toLower(actualHash);
        if (actual != expected)
        {
            throw std::runtime_error("hash mismatch for downloaded file: expected " + expected +
                                     ", got " + actual);
        }

        sol::table result = luaState->create_table();
        result["path"] = destination.generic_string();
        result["bytes"] = static_cast<std::uint64_t>(bytes);
        result["hash"] = actualHash;
        result["verified"] = true;
        return result;
    };
}

}  // namespace beez::plugin::lua
