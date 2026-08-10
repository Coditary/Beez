#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/runtime/context.hpp"

namespace beez::cli
{

void runCacheMaintenance(const ParsedOptions& options,
                         core::BeezSettings& settings,
                         const core::Context& context);

}  // namespace beez::cli
