#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/run/shell_execution.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"
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

    const auto Result = shell_execution_detail::run(*executor,
                                                    command,
                                                    context_,
                                                    runOptions_.outputMode,
                                                    shell_execution_detail::CapturePolicy::Always);
    shell_execution_detail::persistOutput(
        runOptions_, label.detail, "shell", Result.capturedOutput, Result.exitCode);
    shell_execution_detail::logOutput(
        runOptions_, Result.capturedOutput, Result.exitCode, channel);

    if (Result.exitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return Result.exitCode;
}

}  // namespace beez::core
