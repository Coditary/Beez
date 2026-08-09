#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/settings.hpp"

namespace beez::cli
{

void runCacheMaintenance(const ParsedOptions& options,
                         core::BeezSettings& settings,
                         const core::Context& context);

}  // namespace beez::cli
