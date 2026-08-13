#pragma once

#include <cstdint>

namespace beez::core
{

enum class OrchestratorError : std::uint8_t
{
    NotFound,
    AmbiguousStep,
    ExecutionFailed,
    BuildScriptNotFound,
    BuildScriptLoadFailed,
    ExecutorNotAvailable,
    InvalidPhaseRequest,
    StepOrderingFailed,
};

[[nodiscard]] const char* toString(OrchestratorError error);

}  // namespace beez::core
