#pragma once

#include "beez/core/glob_pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/util/expected.hpp"

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

[[nodiscard]] Expected<std::vector<std::vector<Step>>, StepOrderError>
orderStepsInLevels(const std::vector<Step>& steps,
                   const std::vector<StepOrderHint>& hints,
                   const IGlobMatcher& matcher);

// Callback steps may share a single interpreter; split parallel levels so at most one
// callback step runs per level while shell steps stay grouped.
[[nodiscard]] std::vector<std::vector<Step>>
isolateCallbackStepsInLevels(std::vector<std::vector<Step>> levels);

}  // namespace beez::core
