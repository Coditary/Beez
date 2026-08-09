#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_detail.hpp"

#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <oneapi/tbb/flow_graph.h>

namespace beez::core
{

void Orchestrator::recordWorkflowFailure(WorkflowExecutionState& executionState,
                                         OrchestratorError error)
{
    executionState.failed.store(true);
    const std::scoped_lock Lock(executionState.errorMutex);
    executionState.error = error;
}

void Orchestrator::runWorkflowStep(const WorkflowStep& step,
                                   ProgressState& progress,
                                   WorkflowExecutionState& executionState)
{
    beginRunSegment(orchestrator_detail::workflowSegmentLabel(step));

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
