#pragma once

#include "beez/cli/parsed_options.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"

namespace beez::cli
{

[[nodiscard]] int runParsedInvocation(core::Orchestrator& orchestrator,
                                      const core::Registry& registry,
                                      const ParsedOptions& options);

}  // namespace beez::cli
