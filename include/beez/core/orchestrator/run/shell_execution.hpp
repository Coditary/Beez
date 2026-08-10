#pragma once

#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/contract/executor.hpp"

#include <string>

namespace beez::core::shell_execution_detail
{

enum class CapturePolicy
{
    Always,
    UnlessVerbose,
};

struct CommandResult
{
    int exitCode = 0;
    std::string capturedOutput;
};

[[nodiscard]] CommandResult run(plugin::IExecutor& executor,
                                const std::string& command,
                                Context& context,
                                logging::OutputMode outputMode,
                                CapturePolicy capturePolicy);

void persistOutput(const RunOptions& runOptions,
                   const std::string& stepName,
                   const std::string& workerName,
                   const std::string& output,
                   int exitCode);

void logOutput(const RunOptions& runOptions,
               const std::string& output,
               int exitCode,
               logging::LogChannelId channel = {});

}  // namespace beez::core::shell_execution_detail
