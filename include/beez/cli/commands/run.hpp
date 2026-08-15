#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/logging/console/output_mode.hpp"

namespace beez::cli
{

[[nodiscard]] int
runOrchestratorCommand(core::Orchestrator& orchestrator,
                       core::Registry& registry,
                       const core::Context& context,
                       const ParsedOptions& options,
                       logging::OutputMode outputMode = logging::OutputMode::Clean);

}  // namespace beez::cli
