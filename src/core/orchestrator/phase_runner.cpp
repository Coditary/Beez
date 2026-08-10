#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_detail.hpp"

#include "beez/core/config/performance/performance_options.hpp"
#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/registry/step_order.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

namespace beez::core
{

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
    const orchestrator_detail::ThroughputRunScope ThroughputScope(
        pluginHost_, runOptions_.performance.optimizeGcForThroughput);

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
            runOptions_.logger->endRun(true,
                                       orchestrator_detail::elapsedSeconds(Start),
                                       buildRunSummary(orchestrator_detail::elapsedSeconds(Start)));
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
                    false,
                    orchestrator_detail::elapsedSeconds(Start),
                    buildRunSummary(orchestrator_detail::elapsedSeconds(Start)));
            }
            return Result.error();
        }
    }

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(true,
                                   orchestrator_detail::elapsedSeconds(Start),
                                   buildRunSummary(orchestrator_detail::elapsedSeconds(Start)));
    }

    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
    {
        flushBufferedCacheWrites();
    }

    return 0;
}

}  // namespace beez::core
