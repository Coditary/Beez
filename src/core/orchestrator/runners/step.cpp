#include "beez/core/orchestrator/orchestrator.hpp"

#include "beez/core/config/ui/progress_detail.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

#include <chrono>
#include <string>

namespace beez::core
{

Expected<int, OrchestratorError> Orchestrator::runStepInstance(const Step& step,
                                                               ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    const auto Prepare = prepareStepCache(step, progress, Category, Detail);
    if (Prepare.skipped)
    {
        return 0;
    }

    const auto StepStart = std::chrono::steady_clock::now();
    const auto Result = executeStepBody(step, progress, Category, Detail);
    if (!Result)
    {
        return Result.error();
    }

    finalizeStepCache(Prepare.session, step, elapsedSeconds(StepStart));

    return Result.value();
}

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const auto FoundStep = registry_.findStep(name);
    if (!FoundStep)
    {
        return OrchestratorError::NotFound;
    }

    auto runScope = beginLoggedRun("Step", name);
    runScope.beginSegment(name);
    ProgressState progress {.total = 1};
    const auto Result = runStepInstance(*FoundStep, progress);
    runScope.endSegment(static_cast<bool>(Result));
    runScope.finish(static_cast<bool>(Result), workerThreads());

    flushBufferedCacheWritesAtRunEnd();

    return Result;
}

}  // namespace beez::core
