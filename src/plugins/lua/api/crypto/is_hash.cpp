#include "beez/plugin/lua/api/crypto/is_hash.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindIsHash(sol::table& cryptoTable)
{
    cryptoTable["is_hash"] = [](const std::string& algorithm) -> bool
    { return crypto_detail::isHashAlgorithm(algorithm); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
