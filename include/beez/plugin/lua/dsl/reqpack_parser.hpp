#pragma once

#include "beez/core/reqpack/types.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] core::ReqPackManifest parseReqPackTable(const sol::table& table);

}  // namespace beez::plugin::lua
