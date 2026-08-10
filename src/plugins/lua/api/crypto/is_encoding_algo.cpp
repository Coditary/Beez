#include "beez/plugin/lua/api/crypto/is_encoding_algo.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIsEncodingAlgo(sol::table& cryptoTable)
{
    cryptoTable["is_encoding_algo"] = [](const std::string& algorithm) -> bool
    { return crypto_detail::isEncodingAlgorithm(algorithm); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
