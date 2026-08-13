#include "beez/core/orchestrator/orchestrator.hpp"

#include "beez/core/config/ui/progress_detail.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/entry.hpp"
#include "beez/core/orchestrator/run/time.hpp"
#include "beez/core/orchestrator/runners/step.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_resolution.hpp"
#include "beez/core/util/expected.hpp"

#include <chrono>
#include <string>

namespace beez::core::orchestrator_detail
{

Expected<int, OrchestratorError>
runStepInstance(Orchestrator& orchestrator, const Step& step, ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    auto prepare = prepareStepCache(orchestrator, step, progress, Category, Detail);
    if (prepare.skipped)
    {
        return 0;
    }

    const auto StepStart = std::chrono::steady_clock::now();
    const auto Result = executeStepBody(orchestrator, step, progress, Category, Detail);
    if (!Result)
    {
        return Result.error();
    }

    finalizeStepCache(orchestrator, prepare.session, step, elapsedSeconds(StepStart));

    return Result.value();
}

}  // namespace beez::core::orchestrator_detail

namespace beez::core
{

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const auto Resolved = registry_.resolveStep(name);
    if (!Resolved.hasValue())
    {
        if (Resolved.error().error == StepResolutionError::Ambiguous)
        {
            return OrchestratorError::AmbiguousStep;
        }

        return OrchestratorError::NotFound;
    }

    const Step& step = Resolved.value();
    return orchestrator_detail::ScopedLoggedRun(
               *this, "Step", step.name, orchestrator_detail::RunCacheFlushPolicy::AtRunEnd)
        .withSegment(step.name,
                     [&]
                     {
                         ProgressState progress {.total = 1};
                         return orchestrator_detail::runStepInstance(*this, step, progress);
                     });
}

}  // namespace beez::core
