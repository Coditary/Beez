#include "detail/report_helpers.hpp"

#include "beez/core/config/performance/performance_options.hpp"
#include "beez/core/config/report/settings_report.hpp"
#include "beez/core/execution/concurrency/thread_pool.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace beez::core::settings_report
{
void appendPerformanceRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto& cli = input.cliOptions;

    const std::size_t ResolvedThreads =
        ThreadPool(ThreadPoolConfig {.maxThreads = active.performance.maxThreads}).maxConcurrency();
    const PerformanceSettings ResolvedPerformance = active.resolvePerformanceSettings();

    rows.push_back(ConfigRow {
        .key = "performance.max_threads",
        .value = std::to_string(ResolvedThreads),
        .origin = originForOptional(global.performance.maxThreads,
                                    project.performance.maxThreads,
                                    cli.maxThreads.has_value(),
                                    "CLI --threads",
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.cache_write_strategy",
        .value = toString(ResolvedPerformance.cacheWriteStrategy),
        .origin = originForOptional(global.performance.cacheWriteStrategy,
                                    project.performance.cacheWriteStrategy,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.cache_fs_metadata",
        .value = formatBool(ResolvedPerformance.cacheFilesystemMetadata),
        .origin = originForOptional(global.performance.cacheFilesystemMetadata,
                                    project.performance.cacheFilesystemMetadata,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.use_mmap_for_hashing",
        .value = formatBool(ResolvedPerformance.useMmapForHashing),
        .origin = originForOptional(global.performance.useMmapForHashing,
                                    project.performance.useMmapForHashing,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.mmap_hashing_min_bytes",
        .value = std::to_string(ResolvedPerformance.mmapHashingMinBytes),
        .origin = originForOptional(global.performance.mmapHashingMinBytes,
                                    project.performance.mmapHashingMinBytes,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.optimize_gc_for_throughput",
        .value = formatBool(ResolvedPerformance.optimizeGcForThroughput),
        .origin = originForOptional(global.performance.optimizeGcForThroughput,
                                    project.performance.optimizeGcForThroughput,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "performance.pin_threads_to_cores",
        .value = formatBool(ResolvedPerformance.pinThreadsToCores),
        .origin = originForOptional(global.performance.pinThreadsToCores,
                                    project.performance.pinThreadsToCores,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
}
}  // namespace beez::core::settings_report
