#include "beez/core/orchestrator/run/time.hpp"

#include <chrono>

namespace beez::core
{

double elapsedSeconds(const std::chrono::steady_clock::time_point& start)
{
    const auto End = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(End - start).count();
}

}  // namespace beez::core
