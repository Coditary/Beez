#pragma once

#include "beez/core/orchestrator/types.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

class RunStatsTracker
{
  public:
    void reset();
    void beginSegment(std::string label);
    void endSegment(bool success);
    void recordCacheUnit(bool hit, double savedSeconds = 0.0);
    void recordCacheBulk(std::size_t totalUnits, std::size_t hits, double savedSeconds = 0.0);
    void recordPeakWorkers(std::size_t workerCount);

    [[nodiscard]] logging::RunSummary buildSummary(double durationSeconds,
                                                   std::size_t workerThreads) const;

  private:
    std::size_t cacheHitsSkipped_ = 0;
    std::size_t runTotalSteps_ = 0;
    std::size_t peakWorkers_ = 0;
    double cachedTimeSavedSeconds_ = 0.0;
    std::vector<logging::SegmentSummary> runSegments_;
    std::optional<ActiveRunSegment> activeRunSegment_;
};

}  // namespace beez::core
