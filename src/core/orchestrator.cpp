#include "beez/core/orchestrator.h"

#include "beez/core/cache_options.hpp"
#include "beez/core/cache_write_coordinator.hpp"
#include "beez/core/expected.hpp"
#include "beez/core/glob_metadata_cache.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/progress_detail.hpp"
#include "beez/core/run_options.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_cache.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/step_order.hpp"
#include "beez/core/stream_capture.hpp"
#include "beez/core/success_cache.hpp"
#include "beez/core/task.hpp"
#include "beez/core/task_action.hpp"
#include "beez/core/thread_pool.hpp"
#include "beez/core/worker_pool.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/plugin/dsl_loader.hpp"
#include "beez/plugin/plugin_host.h"

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
        cacheHitsSkipped_ = 0;
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Task", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        ProgressState progress {.total = FoundTask->actions.size()};
        const auto Result = runTask(*FoundTask, progress);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(
                static_cast<bool>(Result), elapsedSeconds(Start), runSummary());
        }

        return FlushAtRunEnd(Result);
    }

    if (const auto FoundWorkflow = registry_.findWorkflow(name))
    {
        cacheHitsSkipped_ = 0;
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Workflow", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        const auto Result = runWorkflow(*FoundWorkflow);
        const auto Duration = elapsedSeconds(Start);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(static_cast<bool>(Result), Duration, runSummary());
        }

        return FlushAtRunEnd(Result);
    }

    return OrchestratorError::NotFound;
}

void Orchestrator::logProgress(ProgressState& progress,
                               const std::string& category,
                               const std::string& detail,
                               const bool IsCached) const
{
    if (runOptions_.logger == nullptr)
    {
        return;
    }

    const std::size_t CurrentIndex = progress.index.fetch_add(1) + 1;
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
            ++cacheHitsSkipped_;
            if (!runOptions_.ui.hideCacheHits)
            {
                logProgress(progress, Category, Detail, true);
            }
            else
            {
                (void)progress.index.fetch_add(1);
            }
            return 0;
        }

        outputTracker.emplace(
            context_.projectRoot(), stepCache->matcher(), context_.globMetadataCache());
        outputTracker->begin(step);
    }

    if (const auto& command = step.shellRun)
    {
        const auto Result =
            runShellCommand(*command, {.category = Category, .detail = Detail}, progress, {});
        if (!Result)
        {
            return Result.error();
        }

        if (outputTracker.has_value() && stepCache != nullptr)
        {
            stepCache->store(step, context_.projectRoot(), step.config, outputTracker->end(step));
        }

        return Result.value();
    }

    logProgress(progress, Category, Detail);

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
            [this, &workerFailureOutputs, &workerFailureOutputMutex](
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
                              &threadPool_);
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

        if (outputTracker.has_value() && stepCache != nullptr)
        {
            stepCache->store(step, context_.projectRoot(), step.config, outputTracker->end(step));
        }

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

    cacheHitsSkipped_ = 0;
    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->beginRun("Step", name);
    }

    const auto Start = std::chrono::steady_clock::now();
    ProgressState progress {.total = 1};
    const auto Result = runStepInstance(*FoundStep, progress);

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(static_cast<bool>(Result), elapsedSeconds(Start), runSummary());
    }

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
    return std::accumulate(
        workflow.steps.begin(),
        workflow.steps.end(),
        std::size_t {0},
        [this](std::size_t total, const WorkflowStep& step)
        {
            return total +
                   std::accumulate(step.invocations.begin(),
                                   step.invocations.end(),
                                   std::size_t {0},
                                   [this](std::size_t stepTotal, const PhaseInvocation& invocation)
                                   { return stepTotal + countPhaseInvocationSteps(invocation); });
        });
}

Expected<int, OrchestratorError> Orchestrator::runPhaseInvocation(const PhaseInvocation& invocation,
                                                                  ProgressState& progress)
{
    const auto StepLevels = registry_.stepLevelsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);

    if (!StepLevels.hasValue())
    {
        std::cerr << "Step ordering error: " << StepLevels.error().message << '\n';
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

    cacheHitsSkipped_ = 0;
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
            runOptions_.logger->endRun(true, elapsedSeconds(Start), runSummary());
        }
        return 0;
    }

    ProgressState progress {.total = countPhaseRequestSteps(request)};

    for (const auto& scope : scopes)
    {
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = runPhaseInvocation(Invocation, progress);
        if (!Result)
        {
            if (runOptions_.logger != nullptr)
            {
                runOptions_.logger->endRun(false, elapsedSeconds(Start), runSummary());
            }
            return Result.error();
        }
    }

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(true, elapsedSeconds(Start));
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

void Orchestrator::runParallelWorkflowStep(const WorkflowStep& step,
                                           ProgressState& progress,
                                           WorkflowExecutionState& executionState)
{
    std::vector<logging::LogChannelId> channels(step.invocations.size());
    for (std::size_t index = 0; index < step.invocations.size(); ++index)
    {
        if (runOptions_.logger != nullptr)
        {
            const auto& invocation = step.invocations.at(index);
            channels.at(index) =
                runOptions_.logger->openChannel(invocation.phase + ":" + invocation.scope);
        }
    }

    std::atomic<bool> stepFailed {false};
    OrchestratorError stepError = OrchestratorError::ExecutionFailed;

    threadPool_.parallelFor(step.invocations.size(),
                            [&](std::size_t index)
                            {
                                if (stepFailed.load())
                                {
                                    return;
                                }

                                const auto Result =
                                    runPhaseInvocation(step.invocations.at(index), progress);
                                if (runOptions_.logger != nullptr)
                                {
                                    runOptions_.logger->closeChannel(channels.at(index));
                                }

                                if (!Result)
                                {
                                    stepFailed.store(true);
                                    stepError = Result.error();
                                }
                            });

    if (stepFailed.load())
    {
        recordWorkflowFailure(executionState, stepError);
    }
}

void Orchestrator::runSequentialWorkflowStep(const WorkflowStep& step,
                                             ProgressState& progress,
                                             WorkflowExecutionState& executionState)
{
    logging::LogChannelId channel {};
    if (runOptions_.logger != nullptr)
    {
        const auto& invocation = step.invocations.front();
        channel = runOptions_.logger->openChannel(invocation.phase + ":" + invocation.scope);
    }

    const auto Result = runPhaseInvocation(step.invocations.front(), progress);
    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->closeChannel(channel);
    }

    if (!Result)
    {
        recordWorkflowFailure(executionState, Result.error());
    }
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

                if (step.isParallel())
                {
                    runParallelWorkflowStep(step, progress, executionState);
                }
                else
                {
                    runSequentialWorkflowStep(step, progress, executionState);
                }

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
