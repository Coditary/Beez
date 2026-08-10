#pragma once

#include "beez/core/config/ui/types.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] std::string formatRunSummaryLine(const UiSettings& settings,
                                               const logging::RunSummary& summary);

[[nodiscard]] std::vector<std::string> formatRunEndMessage(const UiSettings& settings,
                                                           bool success,
                                                           double durationSeconds,
                                                           const logging::RunSummary& summary);

}  // namespace beez::core
