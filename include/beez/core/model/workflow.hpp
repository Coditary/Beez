#pragma once

#include "beez/core/model/workflow_stage.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <string>
#include <vector>

namespace beez::core
{

struct Workflow
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::string name;
    std::vector<WorkflowStep> steps;
    std::vector<WorkflowStage> stages;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    [[nodiscard]] bool isStaged() const
    {
        return !stages.empty();
    }
};

}  // namespace beez::core
