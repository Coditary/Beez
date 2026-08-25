#pragma once

#include "beez/core/config/settings/settings.hpp"

#include <filesystem>

namespace beez::plugin::lua
{

[[nodiscard]] bool loadSettingsFromLuaFile(const std::filesystem::path& path,
                                           core::BeezSettings& settings);

void tryLoadGlobalBeezSettings(core::BeezSettings& settings);

void tryLoadProfileBeezSettings(const std::string& profileName, core::BeezSettings& settings);

}  // namespace beez::plugin::lua
