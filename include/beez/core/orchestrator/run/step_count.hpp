#pragma once

#include <cstddef>

namespace beez::core
{

class Orchestrator;
class Workflow;
struct PhaseInvocation;
struct PhaseRequest;

namespace orchestrator_detail
{

[[nodiscard]] std::size_t countWorkflowSteps(const Orchestrator& orchestrator,
                                             const Workflow& workflow);
[[nodiscard]] std::size_t countPhaseInvocationSteps(const Orchestrator& orchestrator,
                                                    const PhaseInvocation& invocation);
[[nodiscard]] std::size_t countPhaseRequestSteps(const Orchestrator& orchestrator,
                                                 const PhaseRequest& request);

}  // namespace orchestrator_detail
}  // namespace beez::core
