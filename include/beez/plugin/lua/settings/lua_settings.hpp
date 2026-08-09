#pragma once

#include "beez/core/config/settings.hpp"

#include <filesystem>

namespace beez::plugin::lua
{

[[nodiscard]] bool loadSettingsFromLuaFile(const std::filesystem::path& path,
                                           core::BeezSettings& settings);

void tryLoadGlobalBeezSettings(core::BeezSettings& settings);

}  // namespace beez::plugin::lua
