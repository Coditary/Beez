#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/orchestrator_internal.hpp"

#include "beez/core/config/ui/progress_detail.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/time.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/util/expected.hpp"

#include <chrono>
#include <string>

namespace beez::core::orchestrator_detail
{

Expected<int, OrchestratorError> runStepInstance(Orchestrator& orchestrator,
                                                 const Step& step,
                                                 ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    auto Prepare = prepareStepCache(orchestrator, step, progress, Category, Detail);
    if (Prepare.skipped)
    {
        return 0;
    }

    const auto StepStart = std::chrono::steady_clock::now();
    const auto Result = executeStepBody(orchestrator, step, progress, Category, Detail);
    if (!Result)
    {
        return Result.error();
    }

    finalizeStepCache(orchestrator, Prepare.session, step, elapsedSeconds(StepStart));

    return Result.value();
}

}  // namespace beez::core::orchestrator_detail

namespace beez::core
{

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const auto FoundStep = registry_.findStep(name);
    if (!FoundStep)
    {
        return OrchestratorError::NotFound;
    }

    return orchestrator_detail::ScopedLoggedRun(
               *this, "Step", name, orchestrator_detail::RunCacheFlushPolicy::AtRunEnd)
        .withSegment(name,
                     [&]
                     {
                         ProgressState progress {.total = 1};
                         return orchestrator_detail::runStepInstance(*this, *FoundStep, progress);
                     });
}

}  // namespace beez::core
