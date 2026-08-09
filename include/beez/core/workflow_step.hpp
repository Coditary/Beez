#pragma once

#include "beez/core/phase_invocation.hpp"

namespace beez::core
{

// A single phase+scope entry in a workflow execution plan.
// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct WorkflowStep
{
    PhaseInvocation invocation;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
