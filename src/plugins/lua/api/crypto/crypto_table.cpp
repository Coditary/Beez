#include "beez/plugin/lua/api/crypto/crypto_table.hpp"

#include "beez/plugin/lua/api/crypto/encode.hpp"
#include "beez/plugin/lua/api/crypto/hash_file.hpp"
#include "beez/plugin/lua/api/crypto/hash_string.hpp"
#include "beez/plugin/lua/api/crypto/is_encoding_algo.hpp"
#include "beez/plugin/lua/api/crypto/is_hash.hpp"
#include "beez/plugin/lua/api/crypto/list_encode_algo.hpp"
#include "beez/plugin/lua/api/crypto/list_hash_algo.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindCrypto(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table cryptoTable = luaState->create_table();
    bindIsHash(cryptoTable);
    bindIsEncodingAlgo(cryptoTable);
    bindListHashAlgo(cryptoTable, luaState);
    bindListEncodeAlgo(cryptoTable, luaState);
    bindHashString(cryptoTable);
    bindHashFile(cryptoTable, context);
    bindEncode(cryptoTable);
    return cryptoTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
