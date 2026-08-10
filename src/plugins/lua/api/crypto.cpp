#include "beez/plugin/lua/api/crypto.hpp"

#include "beez/plugin/lua/api/detail/crypto_ops.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindCrypto(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table cryptoTable = luaState->create_table();

    cryptoTable["is_hash"] = [](const std::string& algorithm) -> bool
    { return api_detail::isHashAlgorithm(algorithm); };

    cryptoTable["is_encoding_algo"] = [](const std::string& algorithm) -> bool
    { return api_detail::isEncodingAlgorithm(algorithm); };

    cryptoTable["hash_string"] =
        [](const std::string& text, sol::optional<std::string> algorithm) -> std::string
    {
        return api_detail::hashString(text, algorithm.value_or("sha256"));
    };

    cryptoTable["hash_file"] =
        [&context](const std::string& path, sol::optional<std::string> algorithm) -> std::string
    {
        return api_detail::hashFile(api_detail::resolvePath(context.projectRoot(), path),
                                    algorithm.value_or("sha256"));
    };

    cryptoTable["encode"] = sol::overload(
        [](const std::string& data) -> std::string { return api_detail::encodeString(data); },
        [](const std::string& data, const std::string& encoding) -> std::string
        {
            if (!api_detail::isEncodingAlgorithm(encoding))
            {
                throw std::runtime_error("beez.crypto.encode: unknown encoding algorithm: " +
                                         encoding);
            }

            return api_detail::encodeString(data, encoding);
        },
        [](const std::string& data, const std::string& key, const std::string& hashAlgorithm)
        { return api_detail::encodeWithKey(data, key, hashAlgorithm); });

    return cryptoTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
