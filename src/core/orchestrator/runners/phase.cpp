#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/orchestrator_internal.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace beez::core::orchestrator_detail
{

Expected<int, OrchestratorError> runPhaseInvocation(Orchestrator& orchestrator,
                                                    const PhaseInvocation& invocation,
                                                    ProgressState& progress)
{
    const auto StepLevels = Access::registry(orchestrator).stepLevelsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);

    if (!StepLevels.hasValue())
    {
        if (logging::writesCliErrorsToConsole(Access::runOptions(orchestrator).outputMode))
        {
            std::cerr << "Step ordering error: " << StepLevels.error().message << '\n';
        }
        flushBufferedCacheWritesForPhase(orchestrator);
        return OrchestratorError::StepOrderingFailed;
    }

    for (const auto& level : StepLevels.value())
    {
        if (level.empty())
        {
            continue;
        }

        if (level.size() == 1 || Access::threadPool(orchestrator).isSequential())
        {
            for (const auto& step : level)
            {
                const auto Result = runStepInstance(orchestrator, step, progress);
                if (!Result)
                {
                    flushBufferedCacheWritesForPhase(orchestrator);
                    return Result.error();
                }
            }
            continue;
        }

        std::atomic<bool> levelFailed {false};
        OrchestratorError levelError = OrchestratorError::ExecutionFailed;
        std::mutex levelErrorMutex;

        Access::threadPool(orchestrator).parallelFor(level.size(),
                                                     [&](std::size_t index)
                                                     {
                                                         if (levelFailed.load())
                                                         {
                                                             return;
                                                         }

                                                         const auto Result =
                                                             runStepInstance(orchestrator,
                                                                             level.at(index),
                                                                             progress);
                                                         if (!Result)
                                                         {
                                                             levelFailed.store(true);
                                                             const std::scoped_lock Lock(
                                                                 levelErrorMutex);
                                                             levelError = Result.error();
                                                         }
                                                     });

        if (levelFailed.load())
        {
            flushBufferedCacheWritesForPhase(orchestrator);
            return levelError;
        }
    }

    flushBufferedCacheWritesForPhase(orchestrator);
    return 0;
}

}  // namespace beez::core::orchestrator_detail

namespace beez::core
{

Expected<int, OrchestratorError> Orchestrator::runPhase(const PhaseRequest& request)
{
    if (request.phase.empty())
    {
        return OrchestratorError::InvalidPhaseRequest;
    }

    auto runScope = orchestrator_detail::beginLoggedRun(*this, "Phase", request.phase);

    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    if (scopes.empty())
    {
        runScope.finish(true, workerThreads());
        return 0;
    }

    ProgressState progress {.total = orchestrator_detail::countPhaseRequestSteps(*this, request)};

    for (const auto& scope : scopes)
    {
        runScope.beginSegment(request.phase + ":" + scope);
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = orchestrator_detail::runPhaseInvocation(*this, Invocation, progress);
        runScope.endSegment(static_cast<bool>(Result));
        if (!Result)
        {
            runScope.finish(false, workerThreads());
            return Result.error();
        }
    }

    runScope.finish(true, workerThreads());

    orchestrator_detail::flushBufferedCacheWritesIfEndStrategy(*this);

    return 0;
}

}  // namespace beez::core
