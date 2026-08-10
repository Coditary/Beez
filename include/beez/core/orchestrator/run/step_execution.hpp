#pragma once

#include "beez/core/cache/step/output_tracker.hpp"
#include "beez/core/model/step.hpp"

#include <optional>

namespace beez::core
{

class StepCache;

namespace step_execution_detail
{

struct StepCacheSession
{
    const StepCache* stepCache = nullptr;
    std::optional<OutputTracker> outputTracker;
};

struct StepCachePrepareResult
{
    bool skipped = false;
    StepCacheSession session;
};

}  // namespace step_execution_detail
}  // namespace beez::core
