#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace beez::logging
{

inline constexpr std::string_view WorkerOutputPrefix = "  | ";

[[nodiscard]] std::size_t stdoutTerminalWidth(std::size_t fallback = 120);

// Splits a worker output line into display segments that fit the terminal width
// including the worker prefix. When terminalWidth is 0, the line is not wrapped.
[[nodiscard]] std::vector<std::string_view> splitWorkerOutputLine(std::string_view line,
                                                                  std::size_t terminalWidth);

}  // namespace beez::logging
