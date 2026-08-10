#pragma once

#include "beez/core/config/ui/types.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace beez::core
{

[[nodiscard]] UiColorPalette defaultColorPalette();

[[nodiscard]] UiColorPalette
resolveThemePalette(const std::optional<std::string>& themeName,
                    const std::map<std::string, UiColorPalette>& themes);

[[nodiscard]] std::string
colorizeText(const UiSettings& settings, const std::string& hexColor, std::string_view text);

}  // namespace beez::core
