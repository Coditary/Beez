#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/types.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace beez::core
{

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
    if (request.phase.empty())
    {
        return OrchestratorError::InvalidPhaseRequest;
    }

    auto runScope = beginLoggedRun("Phase", request.phase);

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

    ProgressState progress {.total = countPhaseRequestSteps(request)};

    for (const auto& scope : scopes)
    {
        runScope.beginSegment(request.phase + ":" + scope);
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = runPhaseInvocation(Invocation, progress);
        runScope.endSegment(static_cast<bool>(Result));
        if (!Result)
        {
            runScope.finish(false, workerThreads());
            return Result.error();
        }
    }

    runScope.finish(true, workerThreads());

    flushBufferedCacheWritesIfEndStrategy();

    return 0;
}

}  // namespace beez::core
