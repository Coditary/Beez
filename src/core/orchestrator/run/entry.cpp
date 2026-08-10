#include "beez/core/orchestrator/run/entry.hpp"

#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/run/cache_flush.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include <string>

namespace beez::core::orchestrator_detail
{

LoggedRunScope
beginLoggedRun(Orchestrator& orchestrator, const std::string& runType, const std::string& name)
{
    const auto& runOptions = Access::runOptions(orchestrator);
    return {Access::pluginHost(orchestrator),
            runOptions.performance.optimizeGcForThroughput,
            Access::stats(orchestrator),
            runOptions.logger,
            runType,
            name};
}

ScopedLoggedRun::ScopedLoggedRun(Orchestrator& orchestrator,
                                 const std::string& runType,
                                 const std::string& name,
                                 const RunCacheFlushPolicy FlushPolicy)
    : orchestrator_(orchestrator), scope_(beginLoggedRun(orchestrator, runType, name)),
      flushPolicy_(FlushPolicy)
{
}

void ScopedLoggedRun::finish(bool success)
{
    if (finished_)
    {
        return;
    }

    finished_ = true;
    scope_.finish(success, orchestrator_.workerThreads());
    flushCache();
}

void ScopedLoggedRun::flushCache()
{
    switch (flushPolicy_)
    {
    case RunCacheFlushPolicy::Never:
        return;
    case RunCacheFlushPolicy::IfEndStrategy:
        flushBufferedCacheWritesIfEndStrategy(orchestrator_);
        return;
    case RunCacheFlushPolicy::AtRunEnd:
        flushBufferedCacheWritesAtRunEnd(orchestrator_);
        return;
    }
}

}  // namespace beez::core::orchestrator_detail
