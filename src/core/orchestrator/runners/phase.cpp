#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/cache_flush.hpp"
#include "beez/core/orchestrator/run/entry.hpp"
#include "beez/core/orchestrator/run/step_count.hpp"
#include "beez/core/orchestrator/runners/step.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/registry/registry.hpp"
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
    const auto StepLevels =
        Access::registry(orchestrator)
            .stepLevelsForPhase(invocation.phase,
                                invocation.scope.empty() ? "*" : invocation.scope);

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

        Access::threadPool(orchestrator)
            .parallelFor(level.size(),
                         [&](std::size_t index)
                         {
                             if (levelFailed.load())
                             {
                                 return;
                             }

                             const auto Result =
                                 runStepInstance(orchestrator, level.at(index), progress);
                             if (!Result)
                             {
                                 levelFailed.store(true);
                                 const std::scoped_lock Lock(levelErrorMutex);
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

    orchestrator_detail::ScopedLoggedRun loggedRun(
        *this, "Phase", request.phase, orchestrator_detail::RunCacheFlushPolicy::Never);

    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    if (scopes.empty())
    {
        loggedRun.finish(true);
        return 0;
    }

    ProgressState progress {.total = orchestrator_detail::countPhaseRequestSteps(*this, request)};

    for (const auto& scope : scopes)
    {
        loggedRun.scope().beginSegment(request.phase + ":" + scope);
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = orchestrator_detail::runPhaseInvocation(*this, Invocation, progress);
        loggedRun.scope().endSegment(static_cast<bool>(Result));
        if (!Result)
        {
            loggedRun.finish(false);
            return Result.error();
        }
    }

    loggedRun.finish(true);
    orchestrator_detail::flushBufferedCacheWritesIfEndStrategy(*this);

    return 0;
}

}  // namespace beez::core
