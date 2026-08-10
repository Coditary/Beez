#include "beez/core/orchestrator/run/shell_execution.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/contract/executor.hpp"
#include <string>

namespace beez::core::shell_execution_detail
{

CommandResult run(plugin::IExecutor& executor,
                  const std::string& command,
                  const Context& context,
                  const logging::OutputMode OutputMode,
                  const CapturePolicy CapturePolicy)
{
    CommandResult result;
    const bool ShouldCapture =
        CapturePolicy == CapturePolicy::Always || OutputMode != logging::OutputMode::Verbose;
    if (ShouldCapture)
    {
        result.exitCode = executor.execute(command, context, &result.capturedOutput);
    }
    else
    {
        result.exitCode = executor.execute(command, context, nullptr);
    }

    return result;
}

void persistOutput(const RunOptions& runOptions,
                   const std::string& stepName,
                   const std::string& workerName,
                   const std::string& output,
                   const int ExitCode)
{
    if (runOptions.runLogWriter != nullptr &&
        runOptions.runLogWriter->shouldPersistWorkerOutput(ExitCode) && !output.empty())
    {
        runOptions.runLogWriter->writeWorkerOutput(stepName, workerName, output, ExitCode);
    }
}

void logOutput(const RunOptions& runOptions,
               const std::string& output,
               const int ExitCode,
               const logging::LogChannelId Channel)
{
    if (runOptions.logger == nullptr || output.empty())
    {
        return;
    }

    if (runOptions.outputMode == logging::OutputMode::Verbose)
    {
        runOptions.logger->logCommandOutput(Channel, output);
    }
    else if (ExitCode != 0)
    {
        runOptions.logger->logFailureOutput(output);
    }
}

}  // namespace beez::core::shell_execution_detail
