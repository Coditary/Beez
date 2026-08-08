#include "beez/core/settings_report.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/env_settings.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/text_table.hpp"
#include "beez/core/thread_pool.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/logging/logging_settings.hpp"
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
    rows.push_back(ConfigRow {
        .key = "ui.summary",
        .value = formatQuoted(toString(ResolvedUi.summaryStyle)),
        .origin = originForOptional(global.ui.options.summary,
                                    project.ui.options.summary,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });

    const logging::LoggingSettings ResolvedLogging = active.resolveLoggingSettings(input.context);
    rows.push_back(ConfigRow {
        .key = "ui.logging.run_log",
        .value = formatBool(ResolvedLogging.runLog),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->runLog
                                                  : std::optional<bool> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->runLog
                                                   : std::optional<bool> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.run_log_file",
        .value = formatQuoted(ResolvedLogging.runLogFile.string()),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->runLogFile
                                                  : std::optional<std::filesystem::path> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->runLogFile
                                                   : std::optional<std::filesystem::path> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.log_steps",
        .value = formatBool(ResolvedLogging.logSteps),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->logSteps
                                                  : std::optional<bool> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->logSteps
                                                   : std::optional<bool> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.workers",
        .value = formatQuoted(logging::toString(ResolvedLogging.workers)),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->workers
                                                  : std::optional<std::string> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->workers
                                                   : std::optional<std::string> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
    rows.push_back(ConfigRow {
        .key = "ui.logging.workers_dir",
        .value = formatQuoted(ResolvedLogging.workersDir.string()),
        .origin = originForOptional(
            global.ui.options.logging.has_value() ? global.ui.options.logging->workersDir
                                                  : std::optional<std::filesystem::path> {},
            project.ui.options.logging.has_value() ? project.ui.options.logging->workersDir
                                                   : std::optional<std::filesystem::path> {},
            false,
            {},
            input.globalConfigPath,
            input.context),
    });
}

[[nodiscard]] std::string formatStringList(const std::vector<std::string>& values)
{
    if (values.empty())
    {
        return "[]";
    }

    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0U)
        {
            stream << ", ";
        }
        stream << formatQuoted(values.at(index));
    }
    stream << ']';
    return stream.str();
}

[[nodiscard]] std::string formatPathList(const std::vector<std::filesystem::path>& paths)
{
    if (paths.empty())
    {
        return "[]";
    }

    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < paths.size(); ++index)
    {
        if (index > 0U)
        {
            stream << ", ";
        }
        stream << '"' << formatDisplayPath(paths.at(index)) << '"';
    }
    stream << ']';
    return stream.str();
}

[[nodiscard]] std::string envVarOriginForKey(const std::string& key,
                                             const BeezSettings& global,
                                             const BeezSettings& project,
                                             const std::filesystem::path& globalConfigPath,
                                             const Context& context)
{
    const bool InProject = project.env.vars.contains(key);
    const bool InGlobal = global.env.vars.contains(key);

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

[[nodiscard]] std::string originForProjectOrGlobal(bool inProject,
                                                   bool inGlobal,
                                                   const std::filesystem::path& globalConfigPath,
                                                   const Context& context)
{
    if (inProject)
    {
        return projectOriginLabel(context);
    }

    if (inGlobal)
    {
        return globalOriginLabel(globalConfigPath);
    }

    return defaultOriginLabel();
}

void appendEnvRows(const SettingsReportInput& input, std::vector<ConfigRow>& rows)
{
    const auto& global = input.globalSettings;
    const auto& project = input.projectSettings;
    const auto& active = input.activeSettings;
    const auto ResolvedEnv = active.resolveEnvSettings();

    rows.push_back(ConfigRow {
        .key = "env.load_dotenv",
        .value = formatBool(ResolvedEnv.loadDotenv),
        .origin = originForOptional(global.env.loadDotenv,
                                    project.env.loadDotenv,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.dotenv_overrides_system",
        .value = formatBool(ResolvedEnv.dotenvOverridesSystem),
        .origin = originForOptional(global.env.dotenvOverridesSystem,
                                    project.env.dotenvOverridesSystem,
                                    false,
                                    {},
                                    input.globalConfigPath,
                                    input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.files",
        .value = formatPathList(ResolvedEnv.files),
        .origin = originForProjectOrGlobal(!project.env.files.empty(),
                                           !global.env.files.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.hash_vars",
        .value = formatStringList(ResolvedEnv.hashVars),
        .origin = originForProjectOrGlobal(!project.env.hashVars.empty(),
                                           !global.env.hashVars.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.ignore_vars_for_hashing",
        .value = formatStringList(ResolvedEnv.ignoreVarsForHashing),
        .origin = originForProjectOrGlobal(!project.env.ignoreVarsForHashing.empty(),
                                           !global.env.ignoreVarsForHashing.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });
    rows.push_back(ConfigRow {
        .key = "env.mask_secrets",
        .value = formatStringList(ResolvedEnv.maskSecrets),
        .origin = originForProjectOrGlobal(!project.env.maskSecrets.empty(),
                                           !global.env.maskSecrets.empty(),
                                           input.globalConfigPath,
                                           input.context),
    });

    std::vector<std::string> keys;
    keys.reserve(ResolvedEnv.vars.size());
    for (const auto& [key, value] : ResolvedEnv.vars)
    {
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    for (const auto& key : keys)
    {
        const auto& value = ResolvedEnv.vars.at(key);
        rows.push_back(ConfigRow {
            .key = "env.vars." + key,
            .value = formatQuoted(value),
            .origin = envVarOriginForKey(key,
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

    std::vector<ConfigRow> envRows;
    appendEnvRows(input, envRows);
    appendSection(stream, "Env", envRows);

    std::string output = stream.str();
    if (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}

}  // namespace beez::core
