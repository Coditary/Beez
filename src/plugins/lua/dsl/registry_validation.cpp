#include "beez/plugin/lua/dsl/registry_validation.hpp"

#include "beez/core/model/task_action.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_resolution.hpp"

#include <stdexcept>
#include <variant>

namespace beez::plugin::lua
{

void validateLoadedRegistry(core::Registry& registry)
{
    for (const auto& [taskName, task] : registry.tasks())
    {
        for (const auto& action : task.actions)
        {
            if (const auto* stepAction = std::get_if<core::TaskStepAction>(&action))
            {
                const auto Resolved = registry.resolveStep(stepAction->stepName);
                if (!Resolved.hasValue())
                {
                    if (Resolved.error().error == core::StepResolutionError::Ambiguous)
                    {
                        throw std::runtime_error("task '" + taskName + "' references ambiguous step '" +
                                                 stepAction->stepName + "'");
                    }

                    throw std::runtime_error("task '" + taskName + "' references undefined step '" +
                                             stepAction->stepName + "'");
                }
            }
        }
    }

    for (const auto& [workflowName, workflow] : registry.workflows())
    {
        for (const auto& workflowStep : workflow.steps)
        {
            const auto& invocation = workflowStep.invocation;
            const auto Matched = registry.stepsForPhase(invocation.phase, invocation.scope);
            if (!Matched.hasValue())
            {
                throw std::runtime_error("workflow '" + workflowName +
                                         "' step ordering failed for phase '" + invocation.phase +
                                         "' scope '" + invocation.scope + "'");
            }

            if (Matched.value().empty())
            {
                throw std::runtime_error("workflow '" + workflowName +
                                         "' has no registered steps for phase '" +
                                         invocation.phase + "' scope '" + invocation.scope + "'");
            }
        }
    }

    registry.validateConsistent();
}

}  // namespace beez::plugin::lua
