#include "beez/core/orchestrator/errors.hpp"

namespace beez::core
{

const char* toString(OrchestratorError error)
{
    switch (error)
    {
    case OrchestratorError::NotFound:
        return "name not found in registry";
    case OrchestratorError::AmbiguousStep:
        return "step name is ambiguous";
    case OrchestratorError::ExecutionFailed:
        return "task execution failed";
    case OrchestratorError::BuildScriptNotFound:
        return "build.lua not found";
    case OrchestratorError::BuildScriptLoadFailed:
        return "failed to load build.lua";
    case OrchestratorError::ExecutorNotAvailable:
        return "no shell executor plugin available";
    case OrchestratorError::InvalidPhaseRequest:
        return "invalid phase request";
    case OrchestratorError::StepOrderingFailed:
        return "step ordering failed";
    }
    return "unknown orchestrator error";
}

}  // namespace beez::core
