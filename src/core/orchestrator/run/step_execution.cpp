#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/run/step_execution.hpp"
#include "orchestrator_internal.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/config/ui/progress_detail.hpp"

#include <string>

namespace beez::core
{

step_execution_detail::StepCachePrepareResult
Orchestrator::prepareStepCache(const Step& step,
                               ProgressState& progress,
                               const std::string& category,
                               const std::string& detail)
{
    step_execution_detail::StepCachePrepareResult result;
    const StepCache* stepCache = runOptions_.stepCache;
    if (stepCache == nullptr || !isStepCacheable(step) || runOptions_.dryRun)
    {
        return result;
    }

    const auto Lookup = stepCache->lookup(step, context_.projectRoot(), step.config);
    if (Lookup.skip)
    {
        recordStepCacheSkip(*this, step, Lookup, progress, category, detail);
        result.skipped = true;
        return result;
    }

    result.session.stepCache = stepCache;
    result.session.outputTracker.emplace(
        context_.projectRoot(), stepCache->matcher(), context_.globMetadataCache());
    result.session.outputTracker->begin(step);
    return result;
}

Expected<int, OrchestratorError> Orchestrator::executeStepBody(const Step& step,
                                                               ProgressState& progress,
                                                               const std::string& category,
                                                               const std::string& detail)
{
    if (const auto& command = step.shellRun)
    {
        return runShellCommand(*command, {.category = category, .detail = detail}, progress, {});
    }

    logProgress(progress, category, detail, false, 0.0, false);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    if (step.hasCallback())
    {
        return step_callback_detail::run(*this, step);
    }

    return OrchestratorError::ExecutionFailed;
}

void Orchestrator::finalizeStepCache(const step_execution_detail::StepCacheSession& session,
                                     const Step& step,
                                     const double durationSeconds)
{
    if (!session.outputTracker.has_value() || session.stepCache == nullptr)
    {
        return;
    }

    session.stepCache->store(step,
                             context_.projectRoot(),
                             step.config,
                             session.outputTracker->end(step),
                             durationSeconds);
}

}  // namespace beez::core
