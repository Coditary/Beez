#pragma once

#include "beez/core/config/settings.hpp"

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

void mergeSettingsFromLuaTable(const sol::table& table, core::BeezSettings& settings);

}  // namespace beez::plugin::lua
