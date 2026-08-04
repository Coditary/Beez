#pragma once

#include "beez/core/phase_invocation.hpp"

#include <vector>

namespace beez::core
{

// A single entry in a workflow execution plan.
// - One invocation: run this phase/scope, then continue to the next step.
// - Multiple invocations: run all listed phases/scopes concurrently, then continue.
// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct WorkflowStep
{
    std::vector<PhaseInvocation> invocations;

    [[nodiscard]] bool isParallel() const noexcept
    {
        return invocations.size() > 1;
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
