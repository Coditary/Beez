#include "beez/core/orchestrator.h"

#include "beez/core/cache/step_cache.hpp"
#include "beez/core/cache/success_cache.hpp"
#include "beez/core/cache/write_coordinator.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"
#include "beez/core/config/run_options.hpp"
#include "beez/core/execution/progress_detail.hpp"
#include "beez/core/execution/stream_capture.hpp"
#include "beez/core/execution/thread_pool.hpp"
#include "beez/core/execution/worker_pool.hpp"
#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/registry/step_order.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <oneapi/tbb/flow_graph.h>

namespace beez::core
{

namespace
{

[[nodiscard]] double elapsedSeconds(const std::chrono::steady_clock::time_point& start)
{
    const auto End = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(End - start).count();
}

class ThroughputRunScope
{
  public:
    ThroughputRunScope(plugin::PluginHost& pluginHost, bool optimizeGc)
        : pluginHost_(pluginHost), active_(optimizeGc)
    {
        if (active_)
        {
            if (auto* dslLoader = pluginHost_.dslLoader())
            {
                dslLoader->setGcThroughputMode(true);
            }
        }
    }

    ThroughputRunScope(const ThroughputRunScope&) = delete;
    ThroughputRunScope& operator=(const ThroughputRunScope&) = delete;
    ThroughputRunScope(ThroughputRunScope&&) = delete;
    ThroughputRunScope& operator=(ThroughputRunScope&&) = delete;

    ~ThroughputRunScope()
    {
        if (!active_)
        {
            return;
        }

        if (auto* dslLoader = pluginHost_.dslLoader())
        {
            dslLoader->setGcThroughputMode(false);
        }
    }

  private:
    plugin::PluginHost& pluginHost_;
    bool active_;
};

}  // namespace

const char* toString(OrchestratorError error)
{
    switch (error)
    {
    case OrchestratorError::NotFound:
        return "name not found in registry";
    case OrchestratorError::ExecutionFailed:
        return "task execution failed";
    case OrchestratorError::BuildScriptNotFound:
        return "build.lua not found";
    case OrchestratorError::BuildScriptLoadFailed:
        return "failed to load build.lua";
    case OrchestratorError::ExecutorNotAvailable:
        return "no shell executor plugin available";
    case OrchestratorError::InvalidPhaseRequest:
        return "invalid phase request";
    case OrchestratorError::StepOrderingFailed:
        return "step ordering failed";
    }
    return "unknown orchestrator error";
}

Orchestrator::Orchestrator(Registry& registry,
                           Context& context,
                           plugin::PluginHost& pluginHost,
                           const RunOptions& runOptions)
    : registry_(registry), context_(context), pluginHost_(pluginHost), runOptions_(runOptions),
      cacheWriteCoordinator_(runOptions.performance.cacheWriteStrategy),
      globMetadataCache_(runOptions.performance.cacheFilesystemMetadata),
      threadPool_(ThreadPoolConfig {.maxThreads = runOptions.maxThreads,
                                    .pinThreadsToCores = runOptions.performance.pinThreadsToCores})
{
    if (runOptions_.enableCache)
    {
        auto cacheOptions = runOptions_.cache;
        if (cacheOptions.root.empty())
        {
            cacheOptions.root = context.projectRoot() / ".cache";
        }

        cacheOptions.writeCoordinator = &cacheWriteCoordinator_;
        runOptions_.cache = cacheOptions;

        if (runOptions_.stepCache == nullptr)
        {
            ownedStepCache_ = std::make_unique<StepCache>(
                cacheOptions, defaultGlobMatcher(), &globMetadataCache_);
            runOptions_.stepCache = ownedStepCache_.get();
        }

        if (runOptions_.successCache == nullptr)
        {
            ownedSuccessCache_ = std::make_unique<SuccessCache>(cacheOptions, defaultGlobMatcher());
            runOptions_.successCache = ownedSuccessCache_.get();
        }
    }

    if (globMetadataCache_.enabled())
    {
        globMetadataCache_.clear();
        context_.setGlobMetadataCache(&globMetadataCache_);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    context_.setCacheStatsRecorder([this](const bool hit, const double savedSeconds)
                                   { recordCacheUnit(hit, savedSeconds); });
}

Orchestrator::~Orchestrator()
{
    flushBufferedCacheWrites();
    context_.clearGlobMetadataCache();
}

Expected<void, OrchestratorError> Orchestrator::loadBuildScript()
{
    const auto ScriptPath = context_.buildScriptPath();
    if (!std::filesystem::exists(ScriptPath))
    {
        return OrchestratorError::BuildScriptNotFound;
    }

    auto* dslLoader = pluginHost_.dslLoader();
    if (dslLoader == nullptr || !dslLoader->load(context_, registry_))
    {
        return OrchestratorError::BuildScriptLoadFailed;
    }

    return {};
}

void Orchestrator::resetRunStats()
{
    cacheHitsSkipped_ = 0;
    runTotalSteps_ = 0;
    peakWorkers_ = 0;
    cachedTimeSavedSeconds_ = 0.0;
    runSegments_.clear();
    activeRunSegment_.reset();
}

void Orchestrator::beginRunSegment(std::string label)
{
    if (activeRunSegment_.has_value())
    {
        endRunSegment(true);
    }

    activeRunSegment_ = ActiveRunSegment {
        .label = std::move(label),
        .started = std::chrono::steady_clock::now(),
    };
}

void Orchestrator::endRunSegment(bool success)
{
    if (!activeRunSegment_.has_value())
    {
        return;
    }

    ActiveRunSegment segment = std::move(*activeRunSegment_);
    activeRunSegment_.reset();

    runSegments_.push_back(logging::SegmentSummary {
        .name = std::move(segment.label),
        .success = success,
        .durationSeconds = elapsedSeconds(segment.started),
        .cacheHits = segment.cacheHits,
        .totalSteps = segment.steps,
    });
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::recordCacheUnit(const bool hit, const double savedSeconds)
{
    ++runTotalSteps_;
    if (hit)
    {
        ++cacheHitsSkipped_;
        if (savedSeconds > 0.0)
        {
            cachedTimeSavedSeconds_ += savedSeconds;
        }
    }

    if (activeRunSegment_.has_value())
    {
        ++activeRunSegment_->steps;
        if (hit)
        {
            ++activeRunSegment_->cacheHits;
        }
    }
}

namespace
{

struct CallbackFileCacheStats
{
    std::size_t units = 0;
    std::size_t hits = 0;
    double savedSeconds = 0.0;
};

[[nodiscard]] CallbackFileCacheStats
estimateCallbackFileCacheStats(const Step& step,
                               const SuccessCache* successCache,
                               const std::filesystem::path& projectRoot,
                               const StepCache* stepCache,
                               GlobMetadataCache* globMetadataCache)
{
    CallbackFileCacheStats stats;
    if (successCache == nullptr || !step.hasCallback())
    {
        return stats;
    }

    std::vector<std::string> patterns = step.input;
    patterns.insert(patterns.end(), step.mutate.begin(), step.mutate.end());
    if (patterns.empty())
    {
        return stats;
    }

    const StepIdentity Identity {.name = step.name, .phase = step.phase, .scope = step.scope};
    const SuccessCacheSession Session =
        successCache->openSession(Identity, projectRoot, step.config);
    const IGlobMatcher& matcher =
        stepCache != nullptr ? stepCache->matcher() : defaultGlobMatcher();
    const auto Files = expandGlobPatterns(patterns, projectRoot, matcher, globMetadataCache);

    // NOLINTNEXTLINE(readability-identifier-naming) -- local set, not a constant
    const std::unordered_set<std::string> uniqueFiles(Files.begin(), Files.end());
    for (const auto& relativePath : uniqueFiles)
    {
        ++stats.units;
        if (Session.fileSuccessCached(relativePath))
        {
            ++stats.hits;
            stats.savedSeconds += Session.fileSavedDurationSeconds(relativePath);
        }
    }

    return stats;
}

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,readability-identifier-naming)
void Orchestrator::recordCacheBulk(const std::size_t totalUnits,
                                   // NOLINTNEXTLINE(readability-identifier-naming)
                                   const std::size_t hits,
                                   // NOLINTNEXTLINE(readability-identifier-naming)
                                   const double savedSeconds)
{
    runTotalSteps_ += totalUnits;
    cacheHitsSkipped_ += hits;
    if (savedSeconds > 0.0)
    {
        cachedTimeSavedSeconds_ += savedSeconds;
    }

    if (activeRunSegment_.has_value())
    {
        activeRunSegment_->steps += totalUnits;
        activeRunSegment_->cacheHits += hits;
    }
}

void Orchestrator::recordStepCacheSkip(const Step& step,
                                       const CacheLookupResult& lookup,
                                       ProgressState& progress,
                                       const std::string& category,
                                       const std::string& detail)
{
    std::size_t totalUnits = 1;
    std::size_t hits = 1;
    double savedSeconds = lookup.savedDurationSeconds;

    if (step.hasCallback() && runOptions_.successCache != nullptr)
    {
        const CallbackFileCacheStats FileStats =
            estimateCallbackFileCacheStats(step,
                                           runOptions_.successCache,
                                           context_.projectRoot(),
                                           runOptions_.stepCache,
                                           context_.globMetadataCache());
        totalUnits = FileStats.units + 1U;
        hits = FileStats.hits + 1U;
        if (savedSeconds <= 0.0 && FileStats.savedSeconds > 0.0)
        {
            savedSeconds = FileStats.savedSeconds;
        }
    }

    recordCacheBulk(totalUnits, hits, savedSeconds);

    if (!runOptions_.ui.hideCacheHits)
    {
        logProgress(progress, category, detail, true, savedSeconds, false);
    }
    else
    {
        (void)progress.index.fetch_add(1);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::recordRunStep(const bool cached)
{
    recordCacheUnit(cached, 0.0);
}

void Orchestrator::recordPeakWorkers(std::size_t workerCount)
{
    peakWorkers_ = std::max(peakWorkers_, workerCount);
}

logging::RunSummary Orchestrator::buildRunSummary(double durationSeconds) const
{
    const std::size_t ExecutedSteps =
        runTotalSteps_ > cacheHitsSkipped_ ? runTotalSteps_ - cacheHitsSkipped_ : 0U;

    double estimatedSaved = cachedTimeSavedSeconds_;
    if (estimatedSaved <= 0.0 && cacheHitsSkipped_ > 0U)
    {
        if (ExecutedSteps > 0U)
        {
            estimatedSaved = durationSeconds / static_cast<double>(ExecutedSteps) *
                             static_cast<double>(cacheHitsSkipped_);
        }
        else if (cacheHitsSkipped_ == runTotalSteps_)
        {
            estimatedSaved = durationSeconds;
        }
    }

    return logging::RunSummary {
        .cacheHitsSkipped = cacheHitsSkipped_,
        .totalSteps = runTotalSteps_,
        .peakWorkers = peakWorkers_,
        .workerThreads = threadPool_.maxConcurrency(),
        .estimatedTimeSavedSeconds = estimatedSaved,
        .segments = runSegments_,
    };
}

void Orchestrator::flushBufferedCacheWrites()
{
    cacheWriteCoordinator_.flush(runOptions_.cache);
}

void Orchestrator::flushBufferedCacheWritesForPhase()
{
    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::Phase)
    {
        flushBufferedCacheWrites();
    }
}

Expected<int, OrchestratorError> Orchestrator::run(const std::string& name)
{
    const ThroughputRunScope ThroughputScope(pluginHost_,
                                             runOptions_.performance.optimizeGcForThroughput);
    const auto FlushAtRunEnd = [this](const auto& result)
    {
        if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
        {
            flushBufferedCacheWrites();
        }
        return result;
    };
    if (const auto FoundTask = registry_.findTask(name))
    {
        resetRunStats();
        beginRunSegment(name);
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Task", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        ProgressState progress {.total = FoundTask->actions.size()};
        const auto Result = runTask(*FoundTask, progress);
        endRunSegment(static_cast<bool>(Result));

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(static_cast<bool>(Result),
                                       elapsedSeconds(Start),
                                       buildRunSummary(elapsedSeconds(Start)));
        }

        return FlushAtRunEnd(Result);
    }

    if (const auto FoundWorkflow = registry_.findWorkflow(name))
    {
        resetRunStats();
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Workflow", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        const auto Result = runWorkflow(*FoundWorkflow);
        const auto Duration = elapsedSeconds(Start);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(
                static_cast<bool>(Result), Duration, buildRunSummary(Duration));
        }

        return FlushAtRunEnd(Result);
    }

    return OrchestratorError::NotFound;
}

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

Expected<int, OrchestratorError> Orchestrator::runTask(const Task& task, ProgressState& progress)
{
    int lastExitCode = 0;
    for (const auto& action : task.actions)
    {
        if (const auto* shellAction = std::get_if<TaskShellAction>(&action))
        {
            const auto Result = runShellCommand(
                shellAction->command,
                {.category = "task", .detail = truncateForDisplay(shellAction->command)},
                progress,
                {});
            if (!Result)
            {
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        const auto* stepAction = std::get_if<TaskStepAction>(&action);
        if (stepAction == nullptr)
        {
            return OrchestratorError::ExecutionFailed;
        }

        const auto FoundStep = registry_.findStep(stepAction->stepName);
        if (!FoundStep)
        {
            return OrchestratorError::NotFound;
        }

        Step step = *FoundStep;
        step.config = mergeStepConfigs(step.config, stepAction->config);

        const auto Result = runStepInstance(step, progress);
        if (!Result)
        {
            return Result.error();
        }
        lastExitCode = Result.value();
    }

    return lastExitCode;
}

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

        StoreCachedOutputs(elapsedSeconds(StepStart));

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

        StoreCachedOutputs(elapsedSeconds(StepStart));

        return exitCode;
    }

    return OrchestratorError::ExecutionFailed;
}

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const ThroughputRunScope ThroughputScope(pluginHost_,
                                             runOptions_.performance.optimizeGcForThroughput);

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
                                   elapsedSeconds(Start),
                                   buildRunSummary(elapsedSeconds(Start)));
    }

    flushBufferedCacheWritesForPhase();
    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
    {
        flushBufferedCacheWrites();
    }

    return Result;
}

std::size_t Orchestrator::countPhaseInvocationSteps(const PhaseInvocation& invocation) const
{
    const auto MatchedSteps = registry_.stepsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);
    if (!MatchedSteps.hasValue())
    {
        return 0;
    }
    return MatchedSteps.value().size();
}

std::size_t Orchestrator::countPhaseRequestSteps(const PhaseRequest& request) const
{
    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    return std::accumulate(scopes.begin(),
                           scopes.end(),
                           std::size_t {0},
                           [this, &request](std::size_t total, const std::string& scope)
                           {
                               return total + countPhaseInvocationSteps(PhaseInvocation {
                                                  .phase = request.phase, .scope = scope});
                           });
}

std::size_t Orchestrator::countWorkflowSteps(const Workflow& workflow) const
{
    return std::accumulate(workflow.steps.begin(),
                           workflow.steps.end(),
                           std::size_t {0},
                           [this](std::size_t total, const WorkflowStep& step)
                           { return total + countPhaseInvocationSteps(step.invocation); });
}

Expected<int, OrchestratorError> Orchestrator::runPhaseInvocation(const PhaseInvocation& invocation,
                                                                  ProgressState& progress)
{
    const auto StepLevels = registry_.stepLevelsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);

    if (!StepLevels.hasValue())
    {
        if (logging::writesCliErrorsToConsole(runOptions_.outputMode))
        {
            std::cerr << "Step ordering error: " << StepLevels.error().message << '\n';
        }
        flushBufferedCacheWritesForPhase();
        return OrchestratorError::StepOrderingFailed;
    }

    for (const auto& level : StepLevels.value())
    {
        if (level.empty())
        {
            continue;
        }

        if (level.size() == 1 || threadPool_.isSequential())
        {
            for (const auto& step : level)
            {
                const auto Result = runStepInstance(step, progress);
                if (!Result)
                {
                    flushBufferedCacheWritesForPhase();
                    return Result.error();
                }
            }
            continue;
        }

        std::atomic<bool> levelFailed {false};
        OrchestratorError levelError = OrchestratorError::ExecutionFailed;
        std::mutex levelErrorMutex;

        threadPool_.parallelFor(level.size(),
                                [&](std::size_t index)
                                {
                                    if (levelFailed.load())
                                    {
                                        return;
                                    }

                                    const auto Result = runStepInstance(level.at(index), progress);
                                    if (!Result)
                                    {
                                        levelFailed.store(true);
                                        const std::scoped_lock Lock(levelErrorMutex);
                                        levelError = Result.error();
                                    }
                                });

        if (levelFailed.load())
        {
            flushBufferedCacheWritesForPhase();
            return levelError;
        }
    }

    flushBufferedCacheWritesForPhase();
    return 0;
}

Expected<int, OrchestratorError> Orchestrator::runPhase(const PhaseRequest& request)
{
    const ThroughputRunScope ThroughputScope(pluginHost_,
                                             runOptions_.performance.optimizeGcForThroughput);

    if (request.phase.empty())
    {
        return OrchestratorError::InvalidPhaseRequest;
    }

    resetRunStats();
    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->beginRun("Phase", request.phase);
    }

    const auto Start = std::chrono::steady_clock::now();

    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    if (scopes.empty())
    {
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(
                true, elapsedSeconds(Start), buildRunSummary(elapsedSeconds(Start)));
        }
        return 0;
    }

    ProgressState progress {.total = countPhaseRequestSteps(request)};

    for (const auto& scope : scopes)
    {
        beginRunSegment(request.phase + ":" + scope);
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = runPhaseInvocation(Invocation, progress);
        endRunSegment(static_cast<bool>(Result));
        if (!Result)
        {
            if (runOptions_.logger != nullptr)
            {
                runOptions_.logger->endRun(
                    false, elapsedSeconds(Start), buildRunSummary(elapsedSeconds(Start)));
            }
            return Result.error();
        }
    }

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(
            true, elapsedSeconds(Start), buildRunSummary(elapsedSeconds(Start)));
    }

    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
    {
        flushBufferedCacheWrites();
    }

    return 0;
}

void Orchestrator::recordWorkflowFailure(WorkflowExecutionState& executionState,
                                         OrchestratorError error)
{
    executionState.failed.store(true);
    const std::scoped_lock Lock(executionState.errorMutex);
    executionState.error = error;
}

[[nodiscard]] std::string workflowSegmentLabel(const WorkflowStep& step)
{
    return step.invocation.phase + ":" + step.invocation.scope;
}

void Orchestrator::runWorkflowStep(const WorkflowStep& step,
                                   ProgressState& progress,
                                   WorkflowExecutionState& executionState)
{
    beginRunSegment(workflowSegmentLabel(step));

    logging::LogChannelId channel {};
    if (runOptions_.logger != nullptr)
    {
        channel =
            runOptions_.logger->openChannel(step.invocation.phase + ":" + step.invocation.scope);
    }

    const auto Result = runPhaseInvocation(step.invocation, progress);
    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->closeChannel(channel);
    }

    if (!Result)
    {
        recordWorkflowFailure(executionState, Result.error());
        endRunSegment(false);
        return;
    }

    endRunSegment(true);
}

Expected<int, OrchestratorError> Orchestrator::runWorkflow(const Workflow& workflow)
{
    if (workflow.steps.empty())
    {
        return 0;
    }

    ProgressState progress {.total = countWorkflowSteps(workflow)};
    WorkflowExecutionState executionState;

    tbb::flow::graph graph;
    using WorkflowNode = tbb::flow::continue_node<tbb::flow::continue_msg>;
    std::vector<std::unique_ptr<WorkflowNode>> nodes;
    nodes.reserve(workflow.steps.size());

    WorkflowNode* predecessor = nullptr;
    for (const auto& workflowStep : workflow.steps)
    {
        auto node = std::make_unique<WorkflowNode>(
            graph,
            [this, step = workflowStep, &progress, &executionState](
                const tbb::flow::continue_msg&) -> tbb::flow::continue_msg
            {
                if (executionState.failed.load())
                {
                    return {};
                }

                runWorkflowStep(step, progress, executionState);

                return {};
            });

        if (predecessor != nullptr)
        {
            tbb::flow::make_edge(*predecessor, *node);
        }

        predecessor = node.get();
        nodes.push_back(std::move(node));
    }

    threadPool_.execute(
        [&]
        {
            nodes.front()->try_put(tbb::flow::continue_msg {});
            graph.wait_for_all();
        });

    if (executionState.failed.load())
    {
        return executionState.error;
    }

    return 0;
}

}  // namespace beez::core
