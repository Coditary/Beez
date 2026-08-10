#include "beez/plugin/lua/api/crypto/hash_string.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindHashString(sol::table& cryptoTable)
{
    cryptoTable["hash_string"] =
        [](const std::string& text, sol::optional<std::string> algorithm) -> std::string
    { return crypto_detail::hashString(text, algorithm.value_or("sha256")); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
