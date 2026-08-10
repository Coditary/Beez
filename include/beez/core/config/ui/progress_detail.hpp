#pragma once

#include "beez/core/model/step.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace beez::core
{

inline constexpr std::size_t DefaultProgressDetailLength = 64;

[[nodiscard]] std::string truncateForDisplay(std::string_view text,
                                             std::size_t maxLength = DefaultProgressDetailLength);

[[nodiscard]] std::string stepProgressDetail(const Step& step);

}  // namespace beez::core
