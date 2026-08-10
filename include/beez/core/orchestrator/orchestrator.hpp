#pragma once

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/step/types.hpp"
#include "beez/core/cache/storage/write_coordinator.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/execution/concurrency/thread_pool.hpp"
#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/orchestrator/run_stats.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace beez::plugin
{
class PluginHost;
}  // namespace beez::plugin

namespace beez::core
{

class StepCache;
class SuccessCache;

enum class OrchestratorError : std::uint8_t
{
    NotFound,
    ExecutionFailed,
    BuildScriptNotFound,
    BuildScriptLoadFailed,
    ExecutorNotAvailable,
    InvalidPhaseRequest,
    StepOrderingFailed,
};

[[nodiscard]] const char* toString(OrchestratorError error);

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

  private:
    struct ProgressState
    {
        std::atomic<std::size_t> index {0};
        std::size_t total = 0;
    };

    struct WorkflowExecutionState
    {
        std::atomic<bool> failed {false};
        OrchestratorError error = OrchestratorError::ExecutionFailed;
        std::mutex errorMutex;
    };

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
    [[nodiscard]] Expected<int, OrchestratorError>
    runPhaseInvocation(const PhaseInvocation& invocation, ProgressState& progress);
    struct ProgressLabel
    {
        std::string category;
        std::string detail;
    };

    [[nodiscard]] Expected<int, OrchestratorError> runShellCommand(const std::string& command,
                                                                   const ProgressLabel& label,
                                                                   ProgressState& progress,
                                                                   logging::LogChannelId channel);

    void logProgress(ProgressState& progress,
                     const std::string& category,
                     const std::string& detail,
                     bool isCached = false,
                     double savedSeconds = 0.0,
                     bool updateCacheStats = true);
    [[nodiscard]] std::size_t countWorkflowSteps(const Workflow& workflow) const;
    [[nodiscard]] std::size_t countPhaseInvocationSteps(const PhaseInvocation& invocation) const;
    [[nodiscard]] std::size_t countPhaseRequestSteps(const PhaseRequest& request) const;

    void flushBufferedCacheWrites();
    void flushBufferedCacheWritesForPhase();

    void resetRunStats();
    void beginRunSegment(std::string label);
    void endRunSegment(bool success);
    void recordRunStep(bool cached);
    void recordCacheUnit(bool hit, double savedSeconds = 0.0);
    void recordCacheBulk(std::size_t totalUnits, std::size_t hits, double savedSeconds = 0.0);
    void recordStepCacheSkip(const Step& step,
                             const CacheLookupResult& lookup,
                             ProgressState& progress,
                             const std::string& category,
                             const std::string& detail);
    void recordPeakWorkers(std::size_t workerCount);
    [[nodiscard]] logging::RunSummary buildRunSummary(double durationSeconds) const;

    std::size_t cacheHitsSkipped_ = 0;
    std::size_t runTotalSteps_ = 0;
    std::size_t peakWorkers_ = 0;
    double cachedTimeSavedSeconds_ = 0.0;
    std::vector<logging::SegmentSummary> runSegments_;
    std::optional<ActiveRunSegment> activeRunSegment_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed kernel
    // dependencies
    Registry& registry_;
    Context& context_;
    plugin::PluginHost& pluginHost_;
    RunOptions runOptions_;
    CacheWriteCoordinator cacheWriteCoordinator_;
    GlobMetadataCache globMetadataCache_;
    ThreadPool threadPool_;
    std::unique_ptr<StepCache> ownedStepCache_;
    std::unique_ptr<SuccessCache> ownedSuccessCache_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace beez::core
