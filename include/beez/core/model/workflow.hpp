#pragma once

#include "beez/core/model/workflow_step.hpp"
#include "beez/core/model/workflow_stage.hpp"

#include <string>
#include <vector>

namespace beez::core
{

struct Workflow
{
    std::string name;
    std::vector<WorkflowStep> steps;
    std::vector<WorkflowStage> stages;

    [[nodiscard]] bool isStaged() const
    {
        return !stages.empty();
    }
};

}  // namespace beez::core
