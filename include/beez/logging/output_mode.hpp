#pragma once

#include <cstdint>

namespace beez::logging
{

enum class OutputMode : std::uint8_t
{
    Clean,
    Verbose,
    Errors,
    Silent,
    // Future: Tui — stack-based terminal UI with bidirectional growth
};

[[nodiscard]] bool writesProgressToConsole(OutputMode mode);
[[nodiscard]] bool writesFailureOutputToConsole(OutputMode mode);
[[nodiscard]] bool writesCliErrorsToConsole(OutputMode mode);
[[nodiscard]] bool writesRunSummaryToConsole(OutputMode mode, bool success);
[[nodiscard]] const char* outputModeToString(OutputMode mode);

}  // namespace beez::logging
