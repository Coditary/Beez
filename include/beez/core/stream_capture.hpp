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

}  // namespace beez::core
