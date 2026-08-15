#pragma once

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
#include <string>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

sol::object runShellCommand(const sol::object& context,
                            const std::string& logPrefix,
                            const std::string& command,
                            const sol::optional<sol::table>& options);

}  // namespace beez::plugin::lua
