#include "detail/report_helpers.hpp"

#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/config/report/settings_report.hpp"

#include <string>
#include <vector>

namespace beez::core::settings_report
{
void appendCacheRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto& cli = input.cliOptions;
    const CacheOptions Resolved = active.resolveCacheOptions(input.context);

    rows.push_back(ConfigRow {
        .key = "cache.enabled",
        .value = formatBool(Resolved.enabled),
        .origin = originForOptional(global.cache.enabled,
                                    project.cache.enabled,
                                    !cli.enableCache,
                                    "CLI --no-cache",
                                    input.globalConfigPath,
                                    input.context),
    });

    std::string cachePathValue;
    if (active.cache.path.has_value())
    {
        cachePathValue = formatQuoted(formatDisplayPath(*active.cache.path));
    }
    else
    {
        cachePathValue = formatQuoted(".cache");
    }

    rows.push_back(ConfigRow {
        .key = "cache.path",
        .value = cachePathValue,
        .origin = originForOptional(global.cache.path,
                                    project.cache.path,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.protect",
        .value = formatBool(Resolved.protect),
        .origin = originForOptional(global.cache.protect,
                                    project.cache.protect,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.hash.algorithm",
        .value = formatQuoted(toString(Resolved.hash.algorithm)),
        .origin = originForOptional(global.cache.hash.algorithm,
                                    project.cache.hash.algorithm,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.hash.seed",
        .value = std::to_string(Resolved.hash.seed),
        .origin = originForOptional(global.cache.hash.seed,
                                    project.cache.hash.seed,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.compress.algorithm",
        .value = formatQuoted(toString(Resolved.compress.algorithm)),
        .origin = originForOptional(global.cache.compress.algorithm,
                                    project.cache.compress.algorithm,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.compress.level",
        .value = std::to_string(Resolved.compress.level),
        .origin = originForOptional(global.cache.compress.level,
                                    project.cache.compress.level,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "cache.compress.mode",
        .value = formatQuoted(toString(Resolved.compress.mode)),
        .origin = originForOptional(global.cache.compress.mode,
                                    project.cache.compress.mode,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
}
}  // namespace beez::core::settings_report
