#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_internal.hpp"
#include "beez/core/orchestrator/run/shell_execution.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/execution/concurrency/worker_pool.hpp"
#include "beez/core/execution/process/stream_capture.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::core::step_callback_detail
{

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Expected<int, OrchestratorError> run(Orchestrator& orchestrator, const Step& step)
{
    auto& context = orchestrator.context();
    const auto& runOptions = orchestrator.runOptions();

    context.setStepConfigAccessor([config = step.config]() -> StepConfigPtr { return config; });
    const StepIdentity Identity {.name = step.name, .phase = step.phase, .scope = step.scope};
    context.setStepIdentity(Identity);

    std::optional<SuccessCacheSession> successCacheSession;
    const SuccessCache* successCache = runOptions.successCache;
    if (successCache != nullptr && !runOptions.dryRun)
    {
        successCacheSession.emplace(
            successCache->openSession(Identity, context.projectRoot(), step.config));
        context.setSuccessCacheSession(&successCacheSession.value());
    }

    struct WorkerFailureOutput
    {
        std::string workerName;
        std::string output;
    };

    std::vector<WorkerFailureOutput> workerFailureOutputs;
    std::mutex workerFailureOutputMutex;

    const StepCache* stepCache = runOptions.stepCache;
    WorkerPool::ExecuteFn executeWorkerCommand =
        [&orchestrator, step, &workerFailureOutputs, &workerFailureOutputMutex](
            const std::string& command, const WorkerSpec& worker) -> int
    {
        auto* executor = orchestrator.pluginHost().executor();
        if (executor == nullptr)
        {
            return -1;
        }

        const auto& options = orchestrator.runOptions();
        const auto Result = shell_execution_detail::run(*executor,
                                                        command,
                                                        orchestrator.context(),
                                                        options.outputMode,
                                                        shell_execution_detail::CapturePolicy::
                                                            UnlessVerbose);
        shell_execution_detail::persistOutput(options,
                                              step.name,
                                              worker.name,
                                              Result.capturedOutput,
                                              Result.exitCode);
        if (Result.exitCode != 0 && !Result.capturedOutput.empty())
        {
            const std::scoped_lock Lock(workerFailureOutputMutex);
            workerFailureOutputs.push_back(WorkerFailureOutput {
                .workerName = worker.name, .output = std::move(Result.capturedOutput)});
        }

        return Result.exitCode;
    };

    WorkerPool workerPool(context.projectRoot(),
                          std::move(executeWorkerCommand),
                          stepCache,
                          stepCache != nullptr ? stepCache->matcher() : defaultGlobMatcher(),
                          step.name,
                          step.config,
                          runOptions.dryRun,
                          &orchestrator.threadPool(),
                          // NOLINTNEXTLINE(readability-identifier-naming)
                          [&orchestrator](const bool hit, const double savedSeconds)
                          { orchestrator.recordCacheUnit(hit, savedSeconds); });
    context.setWorkerPool(&workerPool);

    int exitCode = 0;
    const auto RunCallbackWithWorkers = [&context, &step, &workerPool]() -> int
    {
        const int CallbackExitCode = step.callback(context);
        if (CallbackExitCode != 0)
        {
            return CallbackExitCode;
        }

        return workerPool.drainAll();
    };

    if (runOptions.outputMode == logging::OutputMode::Verbose)
    {
        exitCode = RunCallbackWithWorkers();
    }
    else
    {
        exitCode = discardProcessOutput(RunCallbackWithWorkers);
        if (exitCode != 0 && runOptions.logger != nullptr)
        {
            for (const auto& failure : workerFailureOutputs)
            {
                const logging::LogChannelId Channel =
                    runOptions.logger->openChannel(failure.workerName);
                runOptions.logger->logFailureOutput(failure.output, Channel);
                runOptions.logger->closeChannel(Channel);
            }
        }
    }

    orchestrator.recordPeakWorkers(workerPool.workerCount());

    const std::size_t WorkerCount = workerPool.workerCount();
    if (WorkerCount > 0U)
    {
        const bool AllWorkersCached =
            workerPool.cacheMissCount() == 0U && workerPool.cacheHitCount() == WorkerCount;
        orchestrator.recordCacheUnit(AllWorkersCached, 0.0);
    }
    else
    {
        orchestrator.recordCacheUnit(false, 0.0);
    }

    context.clearWorkerPool();
    context.clearStepConfigAccessor();
    context.clearStepIdentity();
    context.clearSuccessCacheSession();

    if (successCacheSession.has_value())
    {
        successCacheSession->finish();
    }

    if (exitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return exitCode;
}

}  // namespace beez::core::step_callback_detail
