#include "beez/core/model/workflow_resolution.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] bool containsStage(const Workflow& workflow, const std::string& stageName)
{
    for (const WorkflowStage& stage : workflow.stages)
    {
        if (stage.name == stageName)
        {
            return true;
        }
    }

    return false;
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
        for (const PhaseInvocation& invocation : stage.invocations)
        {
            resolved.push_back(WorkflowStep {.invocation = invocation});
        }

        if (targetStage.has_value() && stage.name == *targetStage)
        {
            break;
        }
    }

    return resolved;
}

}  // namespace beez::core
