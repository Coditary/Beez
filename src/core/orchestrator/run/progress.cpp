#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"

#include "beez/core/orchestrator/types.hpp"
#include "beez/logging/contract/logger.hpp"

#include <cstddef>
#include <string>

namespace beez::core
{

// NOLINTNEXTLINE(readability-identifier-naming)
void Orchestrator::logProgress(ProgressState& progress,
                               const std::string& category,
                               const std::string& detail,
                               const bool IsCached,
                               // NOLINTNEXTLINE(readability-identifier-naming)
                               const double savedSeconds,
                               // NOLINTNEXTLINE(readability-identifier-naming)
                               const bool updateCacheStats)
{
    if (updateCacheStats)
    {
        recordCacheUnit(IsCached, IsCached ? savedSeconds : 0.0);
    }
    (void)progress.index.fetch_add(1);

    const auto& runOptions = orchestrator_detail::Access::runOptions(*this);
    if (runOptions.logger == nullptr)
    {
        return;
    }

    const std::size_t CurrentIndex = progress.index.load();
    runOptions.logger->logProgress(logging::ExecutionProgress {
        .index = CurrentIndex,
        .total = progress.total,
        .category = category,
        .detail = detail,
        .cached = IsCached,
    });
}

}  // namespace beez::core
