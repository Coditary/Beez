#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/orchestrator_internal.hpp"

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/shell_execution.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <string>

namespace beez::core::orchestrator_detail
{

Expected<int, OrchestratorError> runShellCommand(Orchestrator& orchestrator,
                                                 const std::string& command,
                                                 const ProgressLabel& label,
                                                 ProgressState& progress,
                                                 logging::LogChannelId channel)
{
    orchestrator.logProgress(progress, label.category, label.detail);

    const auto& runOptions = Access::runOptions(orchestrator);
    if (runOptions.dryRun)
    {
        return 0;
    }

    auto* executor = Access::pluginHost(orchestrator).executor();
    if (executor == nullptr)
    {
        return OrchestratorError::ExecutorNotAvailable;
    }

    const auto Result = shell_execution_detail::run(*executor,
                                                    command,
                                                    Access::context(orchestrator),
                                                    runOptions.outputMode,
                                                    shell_execution_detail::CapturePolicy::Always);
    shell_execution_detail::persistOutput(
        runOptions, label.detail, "shell", Result.capturedOutput, Result.exitCode);
    shell_execution_detail::logOutput(
        runOptions, Result.capturedOutput, Result.exitCode, channel);

    if (Result.exitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return Result.exitCode;
}

}  // namespace beez::core::orchestrator_detail
