#include "beez/core/orchestrator/run/cache_flush.hpp"

#include "beez/core/config/performance/performance_options.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

namespace beez::core::orchestrator_detail
{

void flushBufferedCacheWrites(Orchestrator& orchestrator)
{
    Access::cacheWriteCoordinator(orchestrator).flush(Access::runOptions(orchestrator).cache);
}

void flushBufferedCacheWritesForPhase(Orchestrator& orchestrator)
{
    if (Access::runOptions(orchestrator).performance.cacheWriteStrategy ==
        CacheWriteStrategy::Phase)
    {
        flushBufferedCacheWrites(orchestrator);
    }
}

void flushBufferedCacheWritesIfEndStrategy(Orchestrator& orchestrator)
{
    if (Access::runOptions(orchestrator).performance.cacheWriteStrategy == CacheWriteStrategy::End)
    {
        flushBufferedCacheWrites(orchestrator);
    }
}

void flushBufferedCacheWritesAtRunEnd(Orchestrator& orchestrator)
{
    flushBufferedCacheWritesForPhase(orchestrator);
    flushBufferedCacheWritesIfEndStrategy(orchestrator);
}

}  // namespace beez::core::orchestrator_detail
