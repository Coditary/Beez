#pragma once

#include "beez/core/expected.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace beez::core
{

enum class StepOrderErrorKind : std::uint8_t
{
    MutateConflict,
    Cycle,
};

struct StepOrderError
{
    StepOrderErrorKind kind = StepOrderErrorKind::MutateConflict;
    std::string message;
};

[[nodiscard]] const char* toString(StepOrderErrorKind kind);

struct StepOrderHint
{
    std::string before;
    std::string after;
};

[[nodiscard]] Expected<std::vector<Step>, StepOrderError>
orderSteps(const std::vector<Step>& steps,
           const std::vector<StepOrderHint>& hints,
           const IGlobMatcher& matcher);

}  // namespace beez::core
