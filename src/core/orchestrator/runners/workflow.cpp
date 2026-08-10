#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/orchestrator/run/step_count.hpp"
#include "beez/core/orchestrator/runners/phase.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <oneapi/tbb/flow_graph.h>

namespace beez::core::orchestrator_detail
{

void recordWorkflowFailure(WorkflowExecutionState& executionState, OrchestratorError error)
{
    executionState.failed.store(true);
    const std::scoped_lock Lock(executionState.errorMutex);
    executionState.error = error;
}

void runWorkflowStep(Orchestrator& orchestrator,
                     const WorkflowStep& step,
                     ProgressState& progress,
                     WorkflowExecutionState& executionState)
{
    Access::stats(orchestrator).beginSegment(workflowSegmentLabel(step));

    logging::LogChannelId channel {};
    const auto& runOptions = Access::runOptions(orchestrator);
    if (runOptions.logger != nullptr)
    {
        channel =
            runOptions.logger->openChannel(step.invocation.phase + ":" + step.invocation.scope);
    }

    const auto Result = runPhaseInvocation(orchestrator, step.invocation, progress);
    if (runOptions.logger != nullptr)
    {
        runOptions.logger->closeChannel(channel);
    }

    if (!Result)
    {
        recordWorkflowFailure(executionState, Result.error());
        Access::stats(orchestrator).endSegment(false);
        return;
    }

    Access::stats(orchestrator).endSegment(true);
}

Expected<int, OrchestratorError> runWorkflow(Orchestrator& orchestrator, const Workflow& workflow)
{
    if (workflow.steps.empty())
    {
        return 0;
    }

    ProgressState progress {.total = countWorkflowSteps(orchestrator, workflow)};
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
            [&orchestrator, step = workflowStep, &progress, &executionState](
                const tbb::flow::continue_msg&) -> tbb::flow::continue_msg
            {
                if (executionState.failed.load())
                {
                    return {};
                }

                runWorkflowStep(orchestrator, step, progress, executionState);

                return {};
            });

        if (predecessor != nullptr)
        {
            tbb::flow::make_edge(*predecessor, *node);
        }

        predecessor = node.get();
        nodes.push_back(std::move(node));
    }

    Access::threadPool(orchestrator)
        .execute(
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

}  // namespace beez::core::orchestrator_detail
