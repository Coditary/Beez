#pragma once

#include "beez/core/cache/storage/write_coordinator.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/execution/concurrency/thread_pool.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/stats.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace beez::logging
{
struct LogChannelId;
}

namespace beez::plugin
{
class PluginHost;
}

namespace beez::core
{

class Context;
class Registry;
class Step;
class StepCache;
class SuccessCache;
class Task;
class Workflow;
class WorkflowStep;
struct PhaseInvocation;
struct PhaseRequest;

class LoggedRunScope;

namespace step_execution_detail
{
struct StepCacheSession;
struct StepCachePrepareResult;
}

class Orchestrator
{
  public:
    Orchestrator(Registry& registry,
                 Context& context,
                 plugin::PluginHost& pluginHost,
                 const RunOptions& runOptions = {});
    ~Orchestrator();

    Orchestrator(const Orchestrator&) = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;
    Orchestrator(Orchestrator&&) = delete;
    Orchestrator& operator=(Orchestrator&&) = delete;

    [[nodiscard]] Expected<void, OrchestratorError> loadBuildScript();
    [[nodiscard]] Expected<int, OrchestratorError> run(const std::string& name);
    [[nodiscard]] Expected<int, OrchestratorError> runPhase(const PhaseRequest& request);
    [[nodiscard]] Expected<int, OrchestratorError> runStep(const std::string& name);

    [[nodiscard]] const RunOptions& runOptions() const
    {
        return runOptions_;
    }
    [[nodiscard]] Context& context();
    [[nodiscard]] const Context& context() const;
    [[nodiscard]] plugin::PluginHost& pluginHost();
    [[nodiscard]] ThreadPool& threadPool()
    {
        return threadPool_;
    }
    [[nodiscard]] RunStatsTracker& stats()
    {
        return stats_;
    }
    [[nodiscard]] std::size_t workerThreads() const;

    void recordCacheUnit(bool hit, double savedSeconds = 0.0);
    void recordCacheBulk(std::size_t totalUnits, std::size_t hits, double savedSeconds = 0.0);
    void recordPeakWorkers(std::size_t workerCount);
    void logProgress(ProgressState& progress,
                     const std::string& category,
                     const std::string& detail,
                     bool isCached = false,
                     double savedSeconds = 0.0,
                     bool updateCacheStats = true);

  private:
    void flushBufferedCacheWrites();
    void flushBufferedCacheWritesForPhase();
    void flushBufferedCacheWritesIfEndStrategy();
    void flushBufferedCacheWritesAtRunEnd();

    [[nodiscard]] LoggedRunScope beginLoggedRun(const std::string& runType, const std::string& name);

    [[nodiscard]] Expected<int, OrchestratorError> runTask(const Task& task,
                                                           ProgressState& progress);
    [[nodiscard]] Expected<int, OrchestratorError> runWorkflow(const Workflow& workflow);
    void runWorkflowStep(const WorkflowStep& step,
                         ProgressState& progress,
                         WorkflowExecutionState& executionState);
    static void recordWorkflowFailure(WorkflowExecutionState& executionState,
                                      OrchestratorError error);
    [[nodiscard]] Expected<int, OrchestratorError> runStepInstance(const Step& step,
                                                                   ProgressState& progress);
    [[nodiscard]] step_execution_detail::StepCachePrepareResult
    prepareStepCache(const Step& step,
                     ProgressState& progress,
                     const std::string& category,
                     const std::string& detail);
    [[nodiscard]] Expected<int, OrchestratorError> executeStepBody(const Step& step,
                                                                   ProgressState& progress,
                                                                   const std::string& category,
                                                                   const std::string& detail);
    void finalizeStepCache(const step_execution_detail::StepCacheSession& session,
                           const Step& step,
                           double durationSeconds);
    [[nodiscard]] Expected<int, OrchestratorError>
    runPhaseInvocation(const PhaseInvocation& invocation, ProgressState& progress);
    [[nodiscard]] Expected<int, OrchestratorError> runShellCommand(const std::string& command,
                                                                   const ProgressLabel& label,
                                                                   ProgressState& progress,
                                                                   logging::LogChannelId channel);

    [[nodiscard]] std::size_t countWorkflowSteps(const Workflow& workflow) const;
    [[nodiscard]] std::size_t countPhaseInvocationSteps(const PhaseInvocation& invocation) const;
    [[nodiscard]] std::size_t countPhaseRequestSteps(const PhaseRequest& request) const;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed kernel
    // dependencies
    Registry& registry_;
    Context& context_;
    plugin::PluginHost& pluginHost_;
    RunOptions runOptions_;
    RunStatsTracker stats_;
    CacheWriteCoordinator cacheWriteCoordinator_;
    GlobMetadataCache globMetadataCache_;
    ThreadPool threadPool_;
    std::unique_ptr<StepCache> ownedStepCache_;
    std::unique_ptr<SuccessCache> ownedSuccessCache_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace beez::core
