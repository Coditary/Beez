#pragma once

#include "beez/core/model/workflow_step.hpp"

#include <string>
#include <vector>

namespace beez::core
{

struct Workflow
{
    std::string name;
    std::vector<WorkflowStep> steps;
};

}  // namespace beez::core
