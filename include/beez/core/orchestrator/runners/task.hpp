#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

namespace beez::core
{

class Orchestrator;
class Task;

namespace orchestrator_detail
{

[[nodiscard]] Expected<int, OrchestratorError> runTask(Orchestrator& orchestrator,
                                                       const Task& task,
                                                       ProgressState& progress);

}  // namespace orchestrator_detail
}  // namespace beez::core
