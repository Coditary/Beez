#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"

#include <string>

namespace beez::core
{

class Orchestrator;
class Step;
struct PhaseInvocation;

namespace orchestrator_detail
{

[[nodiscard]] Expected<int, OrchestratorError> runPhaseInvocation(Orchestrator& orchestrator,
                                                                  const PhaseInvocation& invocation,
                                                                  ProgressState& progress);
[[nodiscard]] Expected<int, OrchestratorError> runShellCommand(Orchestrator& orchestrator,
                                                               const std::string& command,
                                                               const ProgressLabel& label,
                                                               ProgressState& progress,
                                                               logging::LogChannelId channel);

}  // namespace orchestrator_detail
}  // namespace beez::core
