#pragma once

#include <chrono>

namespace beez::core
{

[[nodiscard]] double elapsedSeconds(const std::chrono::steady_clock::time_point& start);

}  // namespace beez::core
