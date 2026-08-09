#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace beez::logging
{

struct ExecutionProgress
{
    std::size_t index = 0;
    std::size_t total = 0;
    std::string category;
    std::string detail;
    bool cached = false;
};

struct SegmentSummary
{
    std::string name;
    bool success = true;
    double durationSeconds = 0.0;
    std::size_t cacheHits = 0;
    std::size_t totalSteps = 0;
};

struct RunSummary
{
    std::size_t cacheHitsSkipped = 0;
    std::size_t totalSteps = 0;
    std::size_t peakWorkers = 0;
    std::size_t workerThreads = 0;
    double estimatedTimeSavedSeconds = 0.0;
    std::vector<SegmentSummary> segments;
};

}  // namespace beez::logging
