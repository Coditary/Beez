#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/types.hpp"

#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <cstddef>
#include <string>

namespace beez::core
{

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::logProgress(ProgressState& progress,
                               const std::string& category,
                               const std::string& detail,
                               const bool IsCached,
                               // NOLINTNEXTLINE(readability-identifier-naming)
                               const double savedSeconds,
                               // NOLINTNEXTLINE(readability-identifier-naming)
                               const bool updateCacheStats)
{
    if (updateCacheStats)
    {
        recordCacheUnit(IsCached, IsCached ? savedSeconds : 0.0);
    }
    (void)progress.index.fetch_add(1);

    if (runOptions_.logger == nullptr)
    {
        return;
    }

    const std::size_t CurrentIndex = progress.index.load();
    runOptions_.logger->logProgress(logging::ExecutionProgress {
        .index = CurrentIndex,
        .total = progress.total,
        .category = category,
        .detail = detail,
        .cached = IsCached,
    });
}

Expected<int, OrchestratorError> Orchestrator::runShellCommand(const std::string& command,
                                                               const ProgressLabel& label,
                                                               ProgressState& progress,
                                                               logging::LogChannelId channel)
{
    logProgress(progress, label.category, label.detail);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    auto* executor = pluginHost_.executor();
    if (executor == nullptr)
    {
        return OrchestratorError::ExecutorNotAvailable;
    }

    std::string capturedOutput;
    const int ExitCode = executor->execute(command, context_, &capturedOutput);
    if (runOptions_.runLogWriter != nullptr &&
        runOptions_.runLogWriter->shouldPersistWorkerOutput(ExitCode) && !capturedOutput.empty())
    {
        runOptions_.runLogWriter->writeWorkerOutput(
            label.detail, "shell", capturedOutput, ExitCode);
    }
    if (runOptions_.logger != nullptr && !capturedOutput.empty())
    {
        if (runOptions_.outputMode == logging::OutputMode::Verbose)
        {
            runOptions_.logger->logCommandOutput(channel, capturedOutput);
        }
        else if (ExitCode != 0)
        {
            runOptions_.logger->logFailureOutput(capturedOutput);
        }
    }

    if (ExitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return ExitCode;
}

}  // namespace beez::core
