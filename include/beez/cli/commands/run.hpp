#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/logging/output_mode.hpp"

namespace beez::cli
{

[[nodiscard]] int
runOrchestratorCommand(core::Orchestrator& orchestrator,
                       const core::Registry& registry,
                       const ParsedOptions& options,
                       logging::OutputMode outputMode = logging::OutputMode::Clean);

}  // namespace beez::cli
