#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_detail.hpp"

#include "beez/core/cache/step_cache.hpp"
#include "beez/core/cache/success_cache.hpp"
#include "beez/core/config/performance_options.hpp"
#include "beez/core/execution/progress_detail.hpp"
#include "beez/core/execution/stream_capture.hpp"
#include "beez/core/execution/worker_pool.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- step cache + worker pool branches
Expected<int, OrchestratorError> Orchestrator::runStepInstance(const Step& step,
                                                               ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    const StepCache* stepCache = runOptions_.stepCache;
    std::optional<OutputTracker> outputTracker;
    if (stepCache != nullptr && isStepCacheable(step) && !runOptions_.dryRun)
    {
        const auto Lookup = stepCache->lookup(step, context_.projectRoot(), step.config);
        if (Lookup.skip)
        {
            recordStepCacheSkip(step, Lookup, progress, Category, Detail);
            return 0;
        }

        outputTracker.emplace(
            context_.projectRoot(), stepCache->matcher(), context_.globMetadataCache());
        outputTracker->begin(step);
    }

    const auto StepStart = std::chrono::steady_clock::now();
    const auto StoreCachedOutputs = [&](const double DurationSeconds)
    {
        if (outputTracker.has_value() && stepCache != nullptr)
        {
            stepCache->store(step,
                             context_.projectRoot(),
                             step.config,
                             outputTracker->end(step),
                             DurationSeconds);
        }
    };

    if (const auto& command = step.shellRun)
    {
        const auto Result =
            runShellCommand(*command, {.category = Category, .detail = Detail}, progress, {});
        if (!Result)
        {
            return Result.error();
        }

        StoreCachedOutputs(orchestrator_detail::elapsedSeconds(StepStart));

        return Result.value();
    }

    logProgress(progress, Category, Detail, false, 0.0, false);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    if (step.hasCallback())
    {
        context_.setStepConfigAccessor([config = step.config]() -> StepConfigPtr
                                       { return config; });
        const StepIdentity Identity {.name = step.name, .phase = step.phase, .scope = step.scope};
        context_.setStepIdentity(Identity);

        std::optional<SuccessCacheSession> successCacheSession;
        const SuccessCache* successCache = runOptions_.successCache;
        if (successCache != nullptr && !runOptions_.dryRun)
        {
            successCacheSession.emplace(
                successCache->openSession(Identity, context_.projectRoot(), step.config));
            context_.setSuccessCacheSession(&successCacheSession.value());
        }

        struct WorkerFailureOutput
        {
            std::string workerName;
            std::string output;
        };

        std::vector<WorkerFailureOutput> workerFailureOutputs;
        std::mutex workerFailureOutputMutex;

        WorkerPool::ExecuteFn executeWorkerCommand =
            [this, step, &workerFailureOutputs, &workerFailureOutputMutex](
                const std::string& command, const WorkerSpec& worker) -> int
        {
            auto* executor = pluginHost_.executor();
            if (executor == nullptr)
            {
                return -1;
            }

            if (runOptions_.outputMode == logging::OutputMode::Verbose)
            {
                return executor->execute(command, context_, nullptr);
            }

            std::string capturedOutput;
            const int ExitCode = executor->execute(command, context_, &capturedOutput);
            if (runOptions_.runLogWriter != nullptr &&
                runOptions_.runLogWriter->shouldPersistWorkerOutput(ExitCode) &&
                !capturedOutput.empty())
            {
                runOptions_.runLogWriter->writeWorkerOutput(
                    step.name, worker.name, capturedOutput, ExitCode);
            }
            if (ExitCode != 0 && !capturedOutput.empty())
            {
                const std::scoped_lock Lock(workerFailureOutputMutex);
                workerFailureOutputs.push_back(WorkerFailureOutput {
                    .workerName = worker.name, .output = std::move(capturedOutput)});
            }

            return ExitCode;
        };

        WorkerPool workerPool(context_.projectRoot(),
                              std::move(executeWorkerCommand),
                              stepCache,
                              stepCache != nullptr ? stepCache->matcher() : defaultGlobMatcher(),
                              step.name,
                              step.config,
                              runOptions_.dryRun,
                              &threadPool_,
                              // NOLINTNEXTLINE(readability-identifier-naming)
                              [this](const bool hit, const double savedSeconds)
                              { recordCacheUnit(hit, savedSeconds); });
        context_.setWorkerPool(&workerPool);

        int exitCode = 0;
        const auto RunCallbackWithWorkers = [this, &step, &workerPool]() -> int
        {
            const int CallbackExitCode = step.callback(context_);
            if (CallbackExitCode != 0)
            {
                return CallbackExitCode;
            }

            return workerPool.drainAll();
        };

        if (runOptions_.outputMode == logging::OutputMode::Verbose)
        {
            exitCode = RunCallbackWithWorkers();
        }
        else
        {
            exitCode = discardProcessOutput(RunCallbackWithWorkers);
            if (exitCode != 0 && runOptions_.logger != nullptr)
            {
                for (const auto& failure : workerFailureOutputs)
                {
                    const logging::LogChannelId Channel =
                        runOptions_.logger->openChannel(failure.workerName);
                    runOptions_.logger->logFailureOutput(failure.output, Channel);
                    runOptions_.logger->closeChannel(Channel);
                }
            }
        }

        recordPeakWorkers(workerPool.workerCount());

        const std::size_t WorkerCount = workerPool.workerCount();
        if (WorkerCount > 0U)
        {
            const bool AllWorkersCached =
                workerPool.cacheMissCount() == 0U && workerPool.cacheHitCount() == WorkerCount;
            recordCacheUnit(AllWorkersCached, 0.0);
        }
        else
        {
            recordCacheUnit(false, 0.0);
        }

        context_.clearWorkerPool();
        context_.clearStepConfigAccessor();
        context_.clearStepIdentity();
        context_.clearSuccessCacheSession();

        if (successCacheSession.has_value())
        {
            successCacheSession->finish();
        }

        if (exitCode != 0)
        {
            return OrchestratorError::ExecutionFailed;
        }

        StoreCachedOutputs(orchestrator_detail::elapsedSeconds(StepStart));

        return exitCode;
    }

    return OrchestratorError::ExecutionFailed;
}

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const orchestrator_detail::ThroughputRunScope ThroughputScope(
        pluginHost_, runOptions_.performance.optimizeGcForThroughput);

    const auto FoundStep = registry_.findStep(name);
    if (!FoundStep)
    {
        return OrchestratorError::NotFound;
    }

    resetRunStats();
    beginRunSegment(name);
    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->beginRun("Step", name);
    }

    const auto Start = std::chrono::steady_clock::now();
    ProgressState progress {.total = 1};
    const auto Result = runStepInstance(*FoundStep, progress);
    endRunSegment(static_cast<bool>(Result));

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(static_cast<bool>(Result),
                                   orchestrator_detail::elapsedSeconds(Start),
                                   buildRunSummary(orchestrator_detail::elapsedSeconds(Start)));
    }

    flushBufferedCacheWritesForPhase();
    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
    {
        flushBufferedCacheWrites();
    }

    return Result;
}

}  // namespace beez::core
