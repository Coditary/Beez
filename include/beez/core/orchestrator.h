#pragma once

#include "beez/core/context.h"
#include "beez/core/expected.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/registry.h"
#include "beez/core/run_options.hpp"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/thread_pool.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/logging/logger.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <atomic>

namespace beez::plugin
{
class PluginHost;
}  // namespace beez::plugin

namespace beez::core
{

class StepCache;

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
    void runParallelWorkflowStep(const WorkflowStep& step,
                                 ProgressState& progress,
                                 WorkflowExecutionState& executionState);
    void runSequentialWorkflowStep(const WorkflowStep& step,
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
                     const std::string& detail) const;
    [[nodiscard]] std::size_t countWorkflowSteps(const Workflow& workflow) const;
    [[nodiscard]] std::size_t countPhaseInvocationSteps(const PhaseInvocation& invocation) const;
    [[nodiscard]] std::size_t countPhaseRequestSteps(const PhaseRequest& request) const;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members) -- borrowed kernel
    // dependencies
    Registry& registry_;
    Context& context_;
    plugin::PluginHost& pluginHost_;
    RunOptions runOptions_;
    ThreadPool threadPool_;
    std::unique_ptr<StepCache> ownedStepCache_;
    std::unique_ptr<SuccessCache> ownedSuccessCache_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace beez::core
