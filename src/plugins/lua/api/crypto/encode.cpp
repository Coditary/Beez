#include "beez/plugin/lua/api/crypto/encode.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindEncode(sol::table& cryptoTable)
{
    cryptoTable["encode"] = sol::overload(
        [](const std::string& data) -> std::string { return crypto_detail::encodeString(data); },
        [](const std::string& data, const std::string& encoding) -> std::string
        {
            if (!crypto_detail::isEncodingAlgorithm(encoding))
            {
                throw std::runtime_error("beez.crypto.encode: unknown encoding algorithm: " +
                                         encoding);
            }

            return crypto_detail::encodeString(data, encoding);
        },
        [](const std::string& data, const std::string& key, const std::string& hashAlgorithm)
        { return crypto_detail::encodeWithKey(data, key, hashAlgorithm); });
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
