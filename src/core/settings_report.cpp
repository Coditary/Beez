#include "beez/core/settings_report.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/performance_options.hpp"
#include "beez/core/text_table.hpp"
#include "beez/core/thread_pool.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/logging/output_mode.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

struct ConfigRow
{
    std::string key;
    std::string value;
    std::string origin;
};

[[nodiscard]] std::string homeDirectory()
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        return home;
    }

    return {};
}

[[nodiscard]] std::string formatDisplayPath(const std::filesystem::path& path)
{
    const std::string Home = homeDirectory();
    std::string native = path.string();
    if (!Home.empty() && native.starts_with(Home))
    {
        return std::string("~") + native.substr(Home.size());
    }

    return native;
}

[[nodiscard]] std::string formatOptionalPath(const std::optional<std::filesystem::path>& path)
{
    if (!path.has_value())
    {
        return "<unset>";
    }

    return '"' + formatDisplayPath(*path) + '"';
}

[[nodiscard]] std::string formatBool(bool value)
{
    return value ? "true" : "false";
}

[[nodiscard]] std::string formatQuoted(const std::string& value)
{
    return '"' + value + '"';
}

[[nodiscard]] std::string outputModeLabel(logging::OutputMode mode)
{
    switch (mode)
    {
    case logging::OutputMode::Verbose:
        return "verbose";
    case logging::OutputMode::Clean:
        return "clean";
    }

    return "clean";
}

[[nodiscard]] std::string defaultOriginLabel()
{
    return "Default";
}

[[nodiscard]] std::string globalOriginLabel(const std::filesystem::path& globalConfigPath)
{
    if (globalConfigPath.empty())
    {
        return defaultOriginLabel();
    }

    return formatDisplayPath(globalConfigPath);
}

[[nodiscard]] std::string projectOriginLabel(const Context& context)
{
    return formatDisplayPath(context.buildScriptPath().filename());
}

template <typename T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- layers are semantically distinct
[[nodiscard]] std::string originForOptional(const std::optional<T>& globalValue,
                                            const std::optional<T>& projectValue,
                                            bool cliOverride,
                                            const std::string& cliLabel,
                                            const std::filesystem::path& globalConfigPath,
                                            const Context& context)
{
    if (cliOverride)
    {
        return cliLabel;
    }

    if (projectValue.has_value())
    {
        return projectOriginLabel(context);
    }

    if (globalValue.has_value())
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

void appendSection(std::ostringstream& stream,
                   const std::string& title,
                   const std::vector<ConfigRow>& rows)
{
    if (rows.empty())
    {
        return;
    }

    stream << '[' << title << "]\n";

    TextTable table({"Setting", "Value", "Origin"});
    for (const auto& row : rows)
    {
        table.addRow({row.key, row.value, row.origin});
    }

    stream << table.format() << "\n\n";
}

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

void appendUiRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto& cli = input.cliOptions;

    const logging::OutputMode ResolvedOutputMode =
        active.ui.outputMode.value_or(logging::OutputMode::Clean);
    const UiSettings ResolvedUi = active.resolveUiSettings();

    rows.push_back(ConfigRow {
        .key = "ui.output_mode",
        .value = formatQuoted(outputModeLabel(ResolvedOutputMode)),
        .origin = originForOptional(global.ui.outputMode,
                                    project.ui.outputMode,
                                    cli.verbose,
                                    "CLI --verbose",
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.colors",
        .value = formatBool(ResolvedUi.colors),
        .origin = originForOptional(global.ui.options.colors,
                                    project.ui.options.colors,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.truecolor",
        .value = formatBool(ResolvedUi.truecolor),
        .origin = originForOptional(global.ui.options.truecolor,
                                    project.ui.options.truecolor,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.icons",
        .value = formatBool(ResolvedUi.icons),
        .origin = originForOptional(global.ui.options.icons,
                                    project.ui.options.icons,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.progress",
        .value = formatQuoted(toString(ResolvedUi.animation.progress.style)),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.indicator",
        .value = formatQuoted(toString(ResolvedUi.animation.progress.indicator)),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.animation.indicator_spin_interval",
        .value = std::to_string(ResolvedUi.animation.progress.indicatorSpinIntervalMs),
        .origin = defaultOriginLabel(),
    });
    rows.push_back(ConfigRow {
        .key = "ui.log_level",
        .value = formatQuoted(toString(ResolvedUi.logLevel)),
        .origin = originForOptional(global.ui.options.logLevel,
                                    project.ui.options.logLevel,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.hide_cache_hits",
        .value = formatBool(ResolvedUi.hideCacheHits),
        .origin = originForOptional(global.ui.options.hideCacheHits,
                                    project.ui.options.hideCacheHits,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.prefix",
        .value = formatBool(ResolvedUi.workerPrefixEnabled),
        .origin = originForOptional(global.ui.options.workerPrefix,
                                    project.ui.options.workerPrefix,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.show_time_saved",
        .value = formatBool(ResolvedUi.showTimeSaved),
        .origin = originForOptional(global.ui.options.showTimeSaved,
                                    project.ui.options.showTimeSaved,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
}

void appendPathsRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;

    rows.push_back(ConfigRow {
        .key = "paths.build_script",
        .value = formatQuoted(active.paths.buildScript.value_or("build.lua")),
        .origin = originForOptional(global.paths.buildScript,
                                    project.paths.buildScript,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    const auto EnvFile = active.paths.envFile.value_or(std::filesystem::path(".env"));

    rows.push_back(ConfigRow {
        .key = "paths.env_file",
        .value = formatQuoted(formatDisplayPath(EnvFile)),
        .origin = originForOptional(global.paths.envFile,
                                    project.paths.envFile,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
}

void appendEngineRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto& cli = input.cliOptions;

    rows.push_back(ConfigRow {
        .key = "engine.dry_run",
        .value = formatBool(active.engine.dryRun.value_or(false)),
        .origin = originForOptional(global.engine.dryRun,
                                    project.engine.dryRun,
                                    cli.dryRun,
                                    "CLI --dry-run",
                                    input.globalConfigPath,
                                    input.context),
    });

    rows.push_back(ConfigRow {
        .key = "engine.enable_cache",
        .value = formatBool(active.engine.enableCache.value_or(true)),
        .origin = originForOptional(global.engine.enableCache,
                                    project.engine.enableCache,
                                    !cli.enableCache,
                                    "CLI --no-cache",
                                    input.globalConfigPath,
                                    input.context),
    });
}

[[nodiscard]] std::string environmentOriginForKey(const std::string& key,
                                                  const BeezSettings& global,
                                                  const BeezSettings& project,
                                                  const std::filesystem::path& globalConfigPath,
                                                  const Context& context)
{
    const bool InProject = project.environment.contains(key);
    const bool InGlobal = global.environment.contains(key);

    if (InProject)
    {
        return projectOriginLabel(context);
    }

    if (InGlobal)
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

void appendEnvironmentRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    std::vector<std::string> keys;
    keys.reserve(input.activeSettings.environment.size());
    for (const auto& [key, value] : input.activeSettings.environment)
    {
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    for (const auto& key : keys)
    {
        const auto& value = input.activeSettings.environment.at(key);
        rows.push_back(ConfigRow {
            .key = "environment." + key,
            .value = formatQuoted(value),
            .origin = environmentOriginForKey(key,
                                              input.globalSettings,
                                              input.projectSettings,
                                              input.globalConfigPath,
                                              input.context),
        });
    }
}

}  // namespace

std::string formatActiveConfiguration(const SettingsReportInput& input)
{
    std::ostringstream stream;
    stream << "=== Beez Active Configuration ===\n\n";

    std::vector<ConfigRow> performanceRows;
    appendPerformanceRows(input, performanceRows);
    appendSection(stream, "Performance", performanceRows);

    std::vector<ConfigRow> cacheRows;
    appendCacheRows(input, cacheRows);
    appendSection(stream, "Cache", cacheRows);

    std::vector<ConfigRow> uiRows;
    appendUiRows(input, uiRows);
    appendSection(stream, "UI", uiRows);

    std::vector<ConfigRow> pathsRows;
    appendPathsRows(input, pathsRows);
    appendSection(stream, "Paths", pathsRows);

    std::vector<ConfigRow> engineRows;
    appendEngineRows(input, engineRows);
    appendSection(stream, "Engine", engineRows);

    std::vector<ConfigRow> environmentRows;
    appendEnvironmentRows(input, environmentRows);
    appendSection(stream, "Environment", environmentRows);

    std::string output = stream.str();
    if (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}

}  // namespace beez::core
