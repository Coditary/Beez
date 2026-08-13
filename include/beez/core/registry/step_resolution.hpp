#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace beez::core
{

enum class StepResolutionError : std::uint8_t
{
    NotFound,
    Ambiguous,
};

struct StepResolutionFailure
{
    StepResolutionError error = StepResolutionError::NotFound;
    std::vector<std::string> candidates;
};

}  // namespace beez::core
