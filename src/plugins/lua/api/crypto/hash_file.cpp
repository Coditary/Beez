#include "beez/plugin/lua/api/crypto/hash_file.hpp"

#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"
#include "beez/plugin/lua/api/detail/path.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindHashFile(sol::table& cryptoTable, const core::Context& context)
{
    cryptoTable["hash_file"] =
        [&context](const std::string& path,
                   const sol::optional<std::string>& algorithm) -> std::string
    {
        return crypto_detail::hashFile(api_detail::resolvePath(context.projectRoot(), path),
                                       algorithm.value_or("sha256"));
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
