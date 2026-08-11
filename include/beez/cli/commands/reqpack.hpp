#pragma once

#include "beez/cli/parsing/parsed_options.hpp"

#include <optional>

namespace beez::cli
{

struct LoadedProject;

[[nodiscard]] std::optional<int> runReqPackInstallCommand(const LoadedProject& project,
                                                          const ParsedOptions& options);

[[nodiscard]] std::optional<int> ensureReqPackDependencies(const LoadedProject& project,
                                                           const ParsedOptions& options);

}  // namespace beez::cli
