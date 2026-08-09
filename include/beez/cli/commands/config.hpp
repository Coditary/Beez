#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/settings_report.hpp"

#include <optional>

namespace beez::cli
{

[[nodiscard]] std::optional<int> runEarlyConfigCommands(const ParsedOptions& options);

[[nodiscard]] std::optional<int> runShowConfigCommand(const ParsedOptions& options,
                                                      const core::SettingsReportInput& input);

}  // namespace beez::cli
