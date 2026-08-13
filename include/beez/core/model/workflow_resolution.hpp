#pragma once

#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] std::vector<WorkflowStep>
resolveWorkflowExecutionSteps(const Workflow& workflow,
                              const std::optional<std::string>& targetStage = std::nullopt);

}  // namespace beez::core
