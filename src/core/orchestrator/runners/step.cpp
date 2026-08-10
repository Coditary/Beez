#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_internal.hpp"

#include "beez/core/cache/step/output_tracker.hpp"
#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/config/performance/performance_options.hpp"
#include "beez/core/config/ui/progress_detail.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

#include <chrono>
#include <optional>
#include <string>

namespace beez::core
{

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- step cache + callback branches
Expected<int, OrchestratorError> Orchestrator::runStepInstance(const Step& step,
                                                               ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    const StepCache* stepCache = runOptions_.stepCache;
    std::optional<OutputTracker> outputTracker;
    if (stepCache != nullptr && isStepCacheable(step) && !runOptions_.dryRun)
    {
        const auto Lookup = stepCache->lookup(step, context_.projectRoot(), step.config);
        if (Lookup.skip)
        {
            recordStepCacheSkip(*this, step, Lookup, progress, Category, Detail);
            return 0;
        }

        outputTracker.emplace(
            context_.projectRoot(), stepCache->matcher(), context_.globMetadataCache());
        outputTracker->begin(step);
    }

    const auto StepStart = std::chrono::steady_clock::now();
    const auto StoreCachedOutputs = [&](const double DurationSeconds)
    {
        if (outputTracker.has_value() && stepCache != nullptr)
        {
            stepCache->store(step,
                             context_.projectRoot(),
                             step.config,
                             outputTracker->end(step),
                             DurationSeconds);
        }
    };

    if (const auto& command = step.shellRun)
    {
        const auto Result =
            runShellCommand(*command, {.category = Category, .detail = Detail}, progress, {});
        if (!Result)
        {
            return Result.error();
        }

        StoreCachedOutputs(elapsedSeconds(StepStart));

        return Result.value();
    }

    logProgress(progress, Category, Detail, false, 0.0, false);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    if (step.hasCallback())
    {
        const auto Result = step_callback_detail::run(*this, step);
        if (!Result)
        {
            return Result.error();
        }

        StoreCachedOutputs(elapsedSeconds(StepStart));

        return Result.value();
    }

    return OrchestratorError::ExecutionFailed;
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
