#include "beez/core/orchestrator/run/step_count.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/registry/registry.hpp"

#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

namespace beez::core::orchestrator_detail
{

std::size_t countPhaseInvocationSteps(const Orchestrator& orchestrator,
                                      const PhaseInvocation& invocation)
{
    const auto MatchedSteps =
        Access::registry(orchestrator)
            .stepsForPhase(invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);
    if (!MatchedSteps.hasValue())
    {
        return 0;
    }
    return MatchedSteps.value().size();
}

std::size_t countPhaseRequestSteps(const Orchestrator& orchestrator, const PhaseRequest& request)
{
    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = Access::registry(orchestrator).scopesForPhase(request.phase);
    }

    return std::accumulate(scopes.begin(),
                           scopes.end(),
                           std::size_t {0},
                           [&orchestrator, &request](std::size_t total, const std::string& scope)
                           {
                               return total +
                                      countPhaseInvocationSteps(
                                          orchestrator,
                                          PhaseInvocation {.phase = request.phase, .scope = scope});
                           });
}

std::size_t countWorkflowSteps(const Orchestrator& orchestrator, const Workflow& workflow)
{
    return std::accumulate(
        workflow.steps.begin(),
        workflow.steps.end(),
        std::size_t {0},
        [&orchestrator](std::size_t total, const WorkflowStep& step)
        { return total + countPhaseInvocationSteps(orchestrator, step.invocation); });
}

}  // namespace beez::core::orchestrator_detail
