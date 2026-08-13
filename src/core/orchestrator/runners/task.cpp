#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/config/ui/progress_detail.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/runners/phase.hpp"
#include "beez/core/orchestrator/runners/step.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_resolution.hpp"
#include "beez/core/util/expected.hpp"

#include <variant>

namespace beez::core::orchestrator_detail
{

Expected<int, OrchestratorError>
runTask(Orchestrator& orchestrator, const Task& task, ProgressState& progress)
{
    int lastExitCode = 0;
    for (const auto& action : task.actions)
    {
        if (const auto* shellAction = std::get_if<TaskShellAction>(&action))
        {
            const auto Result = runShellCommand(
                orchestrator,
                shellAction->command,
                {.category = "task", .detail = truncateForDisplay(shellAction->command)},
                progress,
                {});
            if (!Result)
            {
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        const auto* stepAction = std::get_if<TaskStepAction>(&action);
        if (stepAction == nullptr)
        {
            return OrchestratorError::ExecutionFailed;
        }

        const auto ResolvedStep = Access::registry(orchestrator).resolveStep(stepAction->stepName);
        if (!ResolvedStep.hasValue())
        {
            if (ResolvedStep.error().error == StepResolutionError::Ambiguous)
            {
                return OrchestratorError::AmbiguousStep;
            }

            return OrchestratorError::NotFound;
        }

        Step step = ResolvedStep.value();
        step.config = mergeStepConfigs(step.config, stepAction->config);

        const auto Result = runStepInstance(orchestrator, step, progress);
        if (!Result)
        {
            return Result.error();
        }
        lastExitCode = Result.value();
    }

    return lastExitCode;
}

}  // namespace beez::core::orchestrator_detail
