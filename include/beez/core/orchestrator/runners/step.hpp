#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/step_execution.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

#include <string>

namespace beez::core
{

class Orchestrator;
class Step;

namespace orchestrator_detail
{

[[nodiscard]] Expected<int, OrchestratorError>
runStepInstance(Orchestrator& orchestrator, const Step& step, ProgressState& progress);
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
                       step_execution_detail::StepCacheSession& session,
                       const Step& step,
                       double durationSeconds);

}  // namespace orchestrator_detail
}  // namespace beez::core
