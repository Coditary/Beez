#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace beez::core
{

struct ActiveRunSegment
{
    std::string label;
    std::chrono::steady_clock::time_point started;
    std::size_t steps = 0;
    std::size_t cacheHits = 0;
};

}  // namespace beez::core
