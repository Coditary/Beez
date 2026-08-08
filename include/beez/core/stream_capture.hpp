#pragma once

#include <functional>
#include <string>

namespace beez::core
{

struct CapturedExecution
{
    int exitCode = 0;
    std::string output;
};

[[nodiscard]] CapturedExecution captureProcessOutput(const std::function<int()>& action);

// Suppresses stdout/stderr for the duration of the action without buffering output.
// Prefer this over captureProcessOutput when parallel subprocesses may write heavily.
[[nodiscard]] int discardProcessOutput(const std::function<int()>& action);

}  // namespace beez::core
