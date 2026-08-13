#pragma once

#include "beez/core/model/phase_invocation.hpp"

#include <string>
#include <vector>

namespace beez::core
{

struct WorkflowStage
{
    std::string name;
    std::vector<PhaseInvocation> invocations;
};

}  // namespace beez::core
