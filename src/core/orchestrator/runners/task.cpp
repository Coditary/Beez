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

#include <algorithm>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace beez::core::orchestrator_detail
{

namespace
{

[[nodiscard]] Expected<int, OrchestratorError>
runTaskImpl(Orchestrator& orchestrator,
            const Task& task,
            ProgressState& progress,
            std::vector<std::string>& callStack)
{
    if (std::ranges::find(callStack, task.name) != callStack.end())
    {
        return OrchestratorError::TaskCycle;
    }

    callStack.push_back(task.name);

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
                callStack.pop_back();
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        if (const auto* invocation = std::get_if<TaskInvocationAction>(&action))
        {
            const auto FoundTask = Access::registry(orchestrator).findTask(invocation->taskName);
            if (!FoundTask.has_value())
            {
                callStack.pop_back();
                return OrchestratorError::NotFound;
            }

            const auto Result = runTaskImpl(orchestrator, *FoundTask, progress, callStack);
            if (!Result)
            {
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        if (const auto* phaseAction = std::get_if<TaskPhaseAction>(&action))
        {
            const auto Result =
                runPhaseInvocation(orchestrator, phaseAction->invocation, progress);
            if (!Result)
            {
                callStack.pop_back();
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        const auto* stepAction = std::get_if<TaskStepAction>(&action);
        if (stepAction == nullptr)
        {
            callStack.pop_back();
            return OrchestratorError::ExecutionFailed;
        }

        const auto ResolvedStep = Access::registry(orchestrator).resolveStep(stepAction->stepName);
        if (!ResolvedStep.hasValue())
        {
            callStack.pop_back();
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
            callStack.pop_back();
            return Result.error();
        }
        lastExitCode = Result.value();
    }

    callStack.pop_back();
    return lastExitCode;
}

}  // namespace

Expected<int, OrchestratorError>
runTask(Orchestrator& orchestrator, const Task& task, ProgressState& progress)
{
    std::vector<std::string> callStack;
    return runTaskImpl(orchestrator, task, progress, callStack);
}

}  // namespace beez::core::orchestrator_detail
