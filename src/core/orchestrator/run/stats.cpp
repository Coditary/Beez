#include "beez/core/orchestrator/run/stats.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>

namespace beez::core
{

void RunStatsTracker::reset()
{
    cacheHitsSkipped_ = 0;
    runTotalSteps_ = 0;
    peakWorkers_ = 0;
    cachedTimeSavedSeconds_ = 0.0;
    runSegments_.clear();
    activeRunSegment_.reset();
}

void RunStatsTracker::beginSegment(std::string label)
{
    if (activeRunSegment_.has_value())
    {
        endSegment(true);
    }

    activeRunSegment_ = ActiveRunSegment {
        .label = std::move(label),
        .started = std::chrono::steady_clock::now(),
    };
}

void RunStatsTracker::endSegment(bool success)
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
        .durationSeconds = elapsedSeconds(segment.started),
        .cacheHits = segment.cacheHits,
        .totalSteps = segment.steps,
    });
}

// NOLINTNEXTLINE(readability-identifier-naming)
void RunStatsTracker::recordCacheUnit(const bool hit, const double savedSeconds)
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

// NOLINTNEXTLINE(readability-identifier-naming)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,readability-identifier-naming)
void RunStatsTracker::recordCacheBulk(const std::size_t totalUnits,
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

void RunStatsTracker::recordPeakWorkers(std::size_t workerCount)
{
    peakWorkers_ = std::max(peakWorkers_, workerCount);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
logging::RunSummary RunStatsTracker::buildSummary(double durationSeconds,
                                                  std::size_t workerThreads) const
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
        .workerThreads = workerThreads,
        .estimatedTimeSavedSeconds = estimatedSaved,
        .segments = runSegments_,
    };
}

}  // namespace beez::core
