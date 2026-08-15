#include "beez/core/model/workflow_resolution.hpp"
#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_stage.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] bool containsStage(const Workflow& workflow, const std::string& stageName)
{
    return std::ranges::any_of(workflow.stages,
                               [&](const WorkflowStage& stage) { return stage.name == stageName; });
}

}  // namespace

std::vector<WorkflowStep>
resolveWorkflowExecutionSteps(const Workflow& workflow,
                              const std::optional<std::string>& targetStage)
{
    if (!workflow.isStaged())
    {
        if (targetStage.has_value())
        {
            throw std::runtime_error("workflow '" + workflow.name + "' does not define stages");
        }

        return workflow.steps;
    }

    if (workflow.steps.empty() && workflow.stages.empty())
    {
        return {};
    }

    if (targetStage.has_value() && !containsStage(workflow, *targetStage))
    {
        throw std::runtime_error("workflow '" + workflow.name + "' has no stage '" + *targetStage +
                                 "'");
    }

    std::vector<WorkflowStep> resolved;
    for (const WorkflowStage& stage : workflow.stages)
    {
        std::ranges::transform(stage.invocations,
                               std::back_inserter(resolved),
                               [](const PhaseInvocation& invocation)
                               { return WorkflowStep {.invocation = invocation}; });

        if (targetStage.has_value() && stage.name == *targetStage)
        {
            break;
        }
    }

    return resolved;
}

}  // namespace beez::core
