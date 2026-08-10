#pragma once

#include "beez/core/cache/step/types.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/orchestrator/run/step_execution.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"

#include <cstddef>
#include <string>

namespace beez::core
{

class Orchestrator;
class Task;
class Workflow;
class WorkflowStep;
struct PhaseInvocation;
struct PhaseRequest;

void recordStepCacheSkip(Orchestrator& orchestrator,
                         const Step& step,
                         const CacheLookupResult& lookup,
                         ProgressState& progress,
                         const std::string& category,
                         const std::string& detail);

namespace step_callback_detail
{
Expected<int, OrchestratorError> run(Orchestrator& orchestrator, const Step& step);
}  // namespace step_callback_detail

namespace orchestrator_detail
{

void flushBufferedCacheWrites(Orchestrator& orchestrator);
void flushBufferedCacheWritesForPhase(Orchestrator& orchestrator);
void flushBufferedCacheWritesIfEndStrategy(Orchestrator& orchestrator);
void flushBufferedCacheWritesAtRunEnd(Orchestrator& orchestrator);

[[nodiscard]] LoggedRunScope beginLoggedRun(Orchestrator& orchestrator,
                                            const std::string& runType,
                                            const std::string& name);

[[nodiscard]] Expected<int, OrchestratorError> runTask(Orchestrator& orchestrator,
                                                       const Task& task,
                                                       ProgressState& progress);
[[nodiscard]] Expected<int, OrchestratorError> runWorkflow(Orchestrator& orchestrator,
                                                           const Workflow& workflow);
void runWorkflowStep(Orchestrator& orchestrator,
                     const WorkflowStep& step,
                     ProgressState& progress,
                     WorkflowExecutionState& executionState);
void recordWorkflowFailure(WorkflowExecutionState& executionState, OrchestratorError error);

[[nodiscard]] Expected<int, OrchestratorError> runStepInstance(Orchestrator& orchestrator,
                                                               const Step& step,
                                                               ProgressState& progress);
[[nodiscard]] step_execution_detail::StepCachePrepareResult
prepareStepCache(Orchestrator& orchestrator,
                 const Step& step,
                 ProgressState& progress,
                 const std::string& category,
                 const std::string& detail);
[[nodiscard]] Expected<int, OrchestratorError> executeStepBody(Orchestrator& orchestrator,
                                                               const Step& step,
                                                               ProgressState& progress,
                                                               const std::string& category,
                                                               const std::string& detail);
void finalizeStepCache(Orchestrator& orchestrator,
                       const step_execution_detail::StepCacheSession& session,
                       const Step& step,
                       double durationSeconds);

[[nodiscard]] Expected<int, OrchestratorError>
runPhaseInvocation(Orchestrator& orchestrator,
                   const PhaseInvocation& invocation,
                   ProgressState& progress);
[[nodiscard]] Expected<int, OrchestratorError> runShellCommand(Orchestrator& orchestrator,
                                                               const std::string& command,
                                                               const ProgressLabel& label,
                                                               ProgressState& progress,
                                                               logging::LogChannelId channel);

[[nodiscard]] std::size_t countWorkflowSteps(const Orchestrator& orchestrator,
                                             const Workflow& workflow);
[[nodiscard]] std::size_t countPhaseInvocationSteps(const Orchestrator& orchestrator,
                                                    const PhaseInvocation& invocation);
[[nodiscard]] std::size_t countPhaseRequestSteps(const Orchestrator& orchestrator,
                                                 const PhaseRequest& request);

}  // namespace orchestrator_detail
}  // namespace beez::core
