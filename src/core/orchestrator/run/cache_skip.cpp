#include "beez/core/orchestrator/run/cache_skip.hpp"
#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/step/types.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/runtime/context.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace beez::core
{

namespace
{

struct CallbackFileCacheStats
{
    std::size_t units = 0;
    std::size_t hits = 0;
    double savedSeconds = 0.0;
};

[[nodiscard]] CallbackFileCacheStats
estimateCallbackFileCacheStats(const Step& step,
                               const SuccessCache* successCache,
                               const std::filesystem::path& projectRoot,
                               const StepCache* stepCache,
                               GlobMetadataCache* globMetadataCache)
{
    CallbackFileCacheStats stats;
    if (successCache == nullptr || !step.hasCallback())
    {
        return stats;
    }

    std::vector<std::string> patterns = step.input;
    patterns.insert(patterns.end(), step.mutate.begin(), step.mutate.end());
    if (patterns.empty())
    {
        return stats;
    }

    const StepIdentity Identity {.name = step.name, .phase = step.phase, .scope = step.scope};
    const SuccessCacheSession Session =
        successCache->openSession(Identity, projectRoot, step.config);
    const IGlobMatcher& matcher =
        stepCache != nullptr ? stepCache->matcher() : defaultGlobMatcher();
    const auto Files = expandGlobPatterns(patterns, projectRoot, matcher, globMetadataCache);

    // NOLINTNEXTLINE(readability-identifier-naming) -- local set, not a constant
    const std::unordered_set<std::string> uniqueFiles(Files.begin(), Files.end());
    for (const auto& relativePath : uniqueFiles)
    {
        ++stats.units;
        if (Session.fileSuccessCached(relativePath))
        {
            ++stats.hits;
            stats.savedSeconds += Session.fileSavedDurationSeconds(relativePath);
        }
    }

    return stats;
}

}  // namespace

void recordStepCacheSkip(Orchestrator& orchestrator,
                         const Step& step,
                         const CacheLookupResult& lookup,
                         ProgressState& progress,
                         const std::string& category,
                         const std::string& detail)
{
    std::size_t totalUnits = 1;
    std::size_t hits = 1;
    double savedSeconds = lookup.savedDurationSeconds;

    const auto& runOptions = orchestrator_detail::Access::runOptions(orchestrator);
    if (step.hasCallback() && runOptions.successCache != nullptr)
    {
        const auto& context = orchestrator_detail::Access::context(orchestrator);
        const CallbackFileCacheStats FileStats =
            estimateCallbackFileCacheStats(step,
                                           runOptions.successCache,
                                           context.projectRoot(),
                                           runOptions.stepCache,
                                           context.globMetadataCache());
        totalUnits = FileStats.units + 1U;
        hits = FileStats.hits + 1U;
        if (savedSeconds <= 0.0 && FileStats.savedSeconds > 0.0)
        {
            savedSeconds = FileStats.savedSeconds;
        }
    }

    orchestrator.recordCacheBulk(totalUnits, hits, savedSeconds);

    if (!runOptions.ui.hideCacheHits)
    {
        orchestrator.logProgress(progress, category, detail, true, savedSeconds, false);
    }
    else
    {
        (void)progress.index.fetch_add(1);
    }
}

}  // namespace beez::core
