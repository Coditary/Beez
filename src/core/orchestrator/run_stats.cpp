#include "beez/core/orchestrator/run_stats.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_detail.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/step/types.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/glob/expand.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace beez::core
{

void Orchestrator::resetRunStats()
{
    cacheHitsSkipped_ = 0;
    runTotalSteps_ = 0;
    peakWorkers_ = 0;
    cachedTimeSavedSeconds_ = 0.0;
    runSegments_.clear();
    activeRunSegment_.reset();
}

void Orchestrator::beginRunSegment(std::string label)
{
    if (activeRunSegment_.has_value())
    {
        endRunSegment(true);
    }

    activeRunSegment_ = ActiveRunSegment {
        .label = std::move(label),
        .started = std::chrono::steady_clock::now(),
    };
}

void Orchestrator::endRunSegment(bool success)
{
    if (!activeRunSegment_.has_value())
    {
        return;
    }

    ActiveRunSegment segment = std::move(*activeRunSegment_);
    activeRunSegment_.reset();

    runSegments_.push_back(logging::SegmentSummary {
        .name = std::move(segment.label),
        .success = success,
        .durationSeconds = orchestrator_detail::elapsedSeconds(segment.started),
        .cacheHits = segment.cacheHits,
        .totalSteps = segment.steps,
    });
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::recordCacheUnit(const bool hit, const double savedSeconds)
{
    ++runTotalSteps_;
    if (hit)
    {
        ++cacheHitsSkipped_;
        if (savedSeconds > 0.0)
        {
            cachedTimeSavedSeconds_ += savedSeconds;
        }
    }

    if (activeRunSegment_.has_value())
    {
        ++activeRunSegment_->steps;
        if (hit)
        {
            ++activeRunSegment_->cacheHits;
        }
    }
}

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

// NOLINTNEXTLINE(readability-identifier-naming)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,readability-identifier-naming)
void Orchestrator::recordCacheBulk(const std::size_t totalUnits,
                                   // NOLINTNEXTLINE(readability-identifier-naming)
                                   const std::size_t hits,
                                   // NOLINTNEXTLINE(readability-identifier-naming)
                                   const double savedSeconds)
{
    runTotalSteps_ += totalUnits;
    cacheHitsSkipped_ += hits;
    if (savedSeconds > 0.0)
    {
        cachedTimeSavedSeconds_ += savedSeconds;
    }

    if (activeRunSegment_.has_value())
    {
        activeRunSegment_->steps += totalUnits;
        activeRunSegment_->cacheHits += hits;
    }
}

void Orchestrator::recordStepCacheSkip(const Step& step,
                                       const CacheLookupResult& lookup,
                                       ProgressState& progress,
                                       const std::string& category,
                                       const std::string& detail)
{
    std::size_t totalUnits = 1;
    std::size_t hits = 1;
    double savedSeconds = lookup.savedDurationSeconds;

    if (step.hasCallback() && runOptions_.successCache != nullptr)
    {
        const CallbackFileCacheStats FileStats =
            estimateCallbackFileCacheStats(step,
                                           runOptions_.successCache,
                                           context_.projectRoot(),
                                           runOptions_.stepCache,
                                           context_.globMetadataCache());
        totalUnits = FileStats.units + 1U;
        hits = FileStats.hits + 1U;
        if (savedSeconds <= 0.0 && FileStats.savedSeconds > 0.0)
        {
            savedSeconds = FileStats.savedSeconds;
        }
    }

    recordCacheBulk(totalUnits, hits, savedSeconds);

    if (!runOptions_.ui.hideCacheHits)
    {
        logProgress(progress, category, detail, true, savedSeconds, false);
    }
    else
    {
        (void)progress.index.fetch_add(1);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::recordRunStep(const bool cached)
{
    recordCacheUnit(cached, 0.0);
}

void Orchestrator::recordPeakWorkers(std::size_t workerCount)
{
    peakWorkers_ = std::max(peakWorkers_, workerCount);
}

logging::RunSummary Orchestrator::buildRunSummary(double durationSeconds) const
{
    const std::size_t ExecutedSteps =
        runTotalSteps_ > cacheHitsSkipped_ ? runTotalSteps_ - cacheHitsSkipped_ : 0U;

    double estimatedSaved = cachedTimeSavedSeconds_;
    if (estimatedSaved <= 0.0 && cacheHitsSkipped_ > 0U)
    {
        if (ExecutedSteps > 0U)
        {
            estimatedSaved = durationSeconds / static_cast<double>(ExecutedSteps) *
                             static_cast<double>(cacheHitsSkipped_);
        }
        else if (cacheHitsSkipped_ == runTotalSteps_)
        {
            estimatedSaved = durationSeconds;
        }
    }

    return logging::RunSummary {
        .cacheHitsSkipped = cacheHitsSkipped_,
        .totalSteps = runTotalSteps_,
        .peakWorkers = peakWorkers_,
        .workerThreads = threadPool_.maxConcurrency(),
        .estimatedTimeSavedSeconds = estimatedSaved,
        .segments = runSegments_,
    };
}

}  // namespace beez::core
