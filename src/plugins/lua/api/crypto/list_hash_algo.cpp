#include "beez/plugin/lua/api/crypto/list_hash_algo.hpp"

#include "beez/plugin/lua/api/crypto/detail/algorithms_table.hpp"
#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindListHashAlgo(sol::table& cryptoTable, const std::shared_ptr<sol::state>& luaState)
{
    cryptoTable["list_hash_algo"] = [luaState]() -> sol::table
    { return crypto_detail::algorithmsToTable(luaState, crypto_detail::supportedHashAlgorithms()); };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
