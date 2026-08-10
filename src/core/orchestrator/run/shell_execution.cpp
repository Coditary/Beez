#include "beez/core/orchestrator/run/shell_execution.hpp"

namespace beez::core::shell_execution_detail
{

CommandResult run(plugin::IExecutor& executor,
                  const std::string& command,
                  Context& context,
                  const logging::OutputMode outputMode,
                  const CapturePolicy capturePolicy)
{
    CommandResult result;
    const bool ShouldCapture = capturePolicy == CapturePolicy::Always ||
                               outputMode != logging::OutputMode::Verbose;
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
                   const int exitCode)
{
    if (runOptions.runLogWriter != nullptr &&
        runOptions.runLogWriter->shouldPersistWorkerOutput(exitCode) && !output.empty())
    {
        runOptions.runLogWriter->writeWorkerOutput(stepName, workerName, output, exitCode);
    }
}

void logOutput(const RunOptions& runOptions,
               const std::string& output,
               const int exitCode,
               const logging::LogChannelId channel)
{
    if (runOptions.logger == nullptr || output.empty())
    {
        return;
    }

    if (runOptions.outputMode == logging::OutputMode::Verbose)
    {
        runOptions.logger->logCommandOutput(channel, output);
    }
    else if (exitCode != 0)
    {
        runOptions.logger->logFailureOutput(output);
    }
}

}  // namespace beez::core::shell_execution_detail
