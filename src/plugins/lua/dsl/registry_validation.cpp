#include "beez/plugin/lua/dsl/registry_validation.hpp"

#include "beez/core/model/task_action.hpp"
#include "beez/core/model/workflow_resolution.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_resolution.hpp"
#include "beez/plugin/lua/dsl/plugin_config_validation.hpp"
#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"
#include "beez/plugin/lua/dsl/task_cycle_validation.hpp"
#include "beez/plugin/lua/dsl/task_step_reference.hpp"

#include <stdexcept>
#include <string>
#include <variant>

namespace beez::plugin::lua
{

// NOLINTBEGIN(performance-inefficient-string-concatenation)
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void validateLoadedRegistry(core::Registry& registry,
                            const ReqpackBeezPluginCatalog& reqpackBeezPlugins)
{
    for (const auto& [taskName, task] : registry.tasks())
    {
        for (const auto& action : task.actions)
        {
            if (const auto* phaseAction = std::get_if<core::TaskPhaseAction>(&action))
            {
                const auto& invocation = phaseAction->invocation;
                const std::string Scope = invocation.scope.empty() ? "*" : invocation.scope;
                const auto Matched = registry.stepsForPhase(invocation.phase, Scope);
                if (!Matched.hasValue())
                {
                    throw std::runtime_error("task '" + taskName +
                                             "' phase ordering failed for phase '" +
                                             invocation.phase + "' scope '" + Scope + "'");
                }

                if (Matched.value().empty())
                {
                    throw std::runtime_error("task '" + taskName +
                                             "' has no registered steps for phase '" +
                                             invocation.phase + "' scope '" + Scope + "'");
                }

                continue;
            }

            if (const auto* stepAction = std::get_if<core::TaskStepAction>(&action))
            {
                validateTaskPluginStepReference(
                    taskName, stepAction->stepName, registry, reqpackBeezPlugins);

                const auto Resolved = registry.resolveStep(stepAction->stepName);
                if (!Resolved.hasValue())
                {
                    if (Resolved.error().error == core::StepResolutionError::Ambiguous)
                    {
                        throw std::runtime_error("task '" + taskName +
                                                 "' references ambiguous step '" +
                                                 stepAction->stepName + "'");
                    }

                    throw std::runtime_error("task '" + taskName + "' references undefined step '" +
                                             stepAction->stepName + "'");
                }
            }
        }
    }

    validateTaskInvocations(registry);
    validateConfiguredPlugins(registry, reqpackBeezPlugins);

    for (const auto& [workflowName, workflow] : registry.workflows())
    {
        const auto ExecutionSteps = core::resolveWorkflowExecutionSteps(workflow);
        for (const auto& workflowStep : ExecutionSteps)
        {
            const auto& invocation = workflowStep.invocation;
            const std::string Scope = invocation.scope.empty() ? "*" : invocation.scope;
            const auto Matched = registry.stepsForPhase(invocation.phase, Scope);
            if (!Matched.hasValue())
            {
                throw std::runtime_error("workflow '" + workflowName +
                                         "' step ordering failed for phase '" + invocation.phase +
                                         "' scope '" + Scope + "'");
            }

            if (Matched.value().empty())
            {
                throw std::runtime_error("workflow '" + workflowName +
                                         "' has no registered steps for phase '" +
                                         invocation.phase + "' scope '" + Scope + "'");
            }
        }
    }

    registry.validateConsistent();
}
// NOLINTEND(performance-inefficient-string-concatenation)

}  // namespace beez::plugin::lua
