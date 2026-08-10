#include "beez/core/cache/step/output_tracker.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/step_execution.hpp"

#include "beez/core/orchestrator/run/cache_skip.hpp"
#include "beez/core/orchestrator/runners/phase.hpp"
#include "beez/core/orchestrator/runners/step_callback.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"
#include <string>

namespace beez::core::orchestrator_detail
{

step_execution_detail::StepCachePrepareResult prepareStepCache(Orchestrator& orchestrator,
                                                               const Step& step,
                                                               ProgressState& progress,
                                                               const std::string& category,
                                                               const std::string& detail)
{
    step_execution_detail::StepCachePrepareResult result;
    const auto& runOptions = Access::runOptions(orchestrator);
    const StepCache* stepCache = runOptions.stepCache;
    if (stepCache == nullptr || !isStepCacheable(step) || runOptions.dryRun)
    {
        return result;
    }

    const auto& context = Access::context(orchestrator);
    const auto Lookup = stepCache->lookup(step, context.projectRoot(), step.config);
    if (Lookup.skip)
    {
        recordStepCacheSkip(orchestrator, step, Lookup, progress, category, detail);
        result.skipped = true;
        return result;
    }

    result.session.stepCache = stepCache;
    result.session.outputTracker.emplace(
        context.projectRoot(), stepCache->matcher(), context.globMetadataCache());
    result.session.outputTracker->begin(step);
    return result;
}

Expected<int, OrchestratorError> executeStepBody(Orchestrator& orchestrator,
                                                 const Step& step,
                                                 ProgressState& progress,
                                                 const std::string& category,
                                                 const std::string& detail)
{
    if (const auto& command = step.shellRun)
    {
        return runShellCommand(
            orchestrator, *command, {.category = category, .detail = detail}, progress, {});
    }

    orchestrator.logProgress(progress, category, detail, false, 0.0, false);

    if (Access::runOptions(orchestrator).dryRun)
    {
        return 0;
    }

    if (step.hasCallback())
    {
        return step_callback_detail::run(orchestrator, step);
    }

    return OrchestratorError::ExecutionFailed;
}

void finalizeStepCache(Orchestrator& orchestrator,
                       step_execution_detail::StepCacheSession& session,
                       const Step& step,
                       const double DurationSeconds)
{
    if (!session.outputTracker.has_value() || session.stepCache == nullptr)
    {
        return;
    }

    const auto& context = Access::context(orchestrator);
    session.stepCache->store(step,
                             context.projectRoot(),
                             step.config,
                             session.outputTracker->end(step),
                             DurationSeconds);
}

}  // namespace beez::core::orchestrator_detail
