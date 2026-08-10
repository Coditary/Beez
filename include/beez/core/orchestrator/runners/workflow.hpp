#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

namespace beez::core
{

class Orchestrator;
class Workflow;
class WorkflowStep;

namespace orchestrator_detail
{

[[nodiscard]] Expected<int, OrchestratorError> runWorkflow(Orchestrator& orchestrator,
                                                           const Workflow& workflow);
void runWorkflowStep(Orchestrator& orchestrator,
                     const WorkflowStep& step,
                     ProgressState& progress,
                     WorkflowExecutionState& executionState);
void recordWorkflowFailure(WorkflowExecutionState& executionState, OrchestratorError error);

}  // namespace orchestrator_detail
}  // namespace beez::core
