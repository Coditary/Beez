#include "beez/core/orchestrator/orchestrator.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

namespace beez::core
{

std::size_t Orchestrator::countPhaseInvocationSteps(const PhaseInvocation& invocation) const
{
    const auto MatchedSteps = registry_.stepsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);
    if (!MatchedSteps.hasValue())
    {
        return 0;
    }
    return MatchedSteps.value().size();
}

std::size_t Orchestrator::countPhaseRequestSteps(const PhaseRequest& request) const
{
    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    return std::accumulate(scopes.begin(),
                           scopes.end(),
                           std::size_t {0},
                           [this, &request](std::size_t total, const std::string& scope)
                           {
                               return total + countPhaseInvocationSteps(PhaseInvocation {
                                                  .phase = request.phase, .scope = scope});
                           });
}

std::size_t Orchestrator::countWorkflowSteps(const Workflow& workflow) const
{
    return std::accumulate(workflow.steps.begin(),
                           workflow.steps.end(),
                           std::size_t {0},
                           [this](std::size_t total, const WorkflowStep& step)
                           { return total + countPhaseInvocationSteps(step.invocation); });
}

}  // namespace beez::core
