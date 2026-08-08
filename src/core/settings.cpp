#include "beez/core/settings.hpp"

#include "beez/cli/parsed_options.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/env_settings.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/run_options.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/logging/logging_settings.hpp"
#include "beez/logging/output_mode.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace beez::core
{

namespace
{

void mergeOptionalPath(std::optional<std::filesystem::path>& target,
                       const std::optional<std::filesystem::path>& overlay)
{
    if (overlay.has_value())
    {
        target = overlay;
    }
}

void mergeOptionalString(std::optional<std::string>& target,
                         const std::optional<std::string>& overlay)
{
    if (overlay.has_value())
    {
        target = overlay;
    }
}

template <typename T>
void mergeOptionalValue(std::optional<T>& target, const std::optional<T>& overlay)
{
    if (overlay.has_value())
    {
        target = overlay;
    }
}

[[nodiscard]] ContentHashSettings resolveHashSettings(const BeezSettings::Cache& cache)
{
    ContentHashSettings settings;
    if (cache.hash.algorithm.has_value())
    {
        settings.algorithm = parseContentHashAlgorithm(*cache.hash.algorithm);
    }
    if (cache.hash.seed.has_value())
    {
        settings.seed = *cache.hash.seed;
    }
    return normalizeContentHashSettings(settings);
}

[[nodiscard]] CacheCompressionSettings resolveCompressionSettings(const BeezSettings::Cache& cache)
{
    CacheCompressionSettings settings;
    if (cache.compress.algorithm.has_value())
    {
        settings.algorithm = parseCacheCompressionAlgorithm(*cache.compress.algorithm);
    }
    if (cache.compress.level.has_value())
    {
        settings.level = *cache.compress.level;
    }
    if (cache.compress.mode.has_value())
    {
        settings.mode = parseCacheCompressionMode(*cache.compress.mode);
    }
    return normalizeCacheCompressionSettings(settings);
}

[[nodiscard]] PerformanceSettings buildPerformanceSettings(const BeezSettings& settings)
{
    PerformanceSettings resolved;
    if (settings.performance.cacheWriteStrategy.has_value())
    {
        resolved.cacheWriteStrategy =
            parseCacheWriteStrategy(*settings.performance.cacheWriteStrategy);
    }
    if (settings.performance.cacheFilesystemMetadata.has_value())
    {
        resolved.cacheFilesystemMetadata = *settings.performance.cacheFilesystemMetadata;
    }
    if (settings.performance.useMmapForHashing.has_value())
    {
        resolved.useMmapForHashing = *settings.performance.useMmapForHashing;
    }
    if (settings.performance.mmapHashingMinBytes.has_value())
    {
        resolved.mmapHashingMinBytes = *settings.performance.mmapHashingMinBytes;
    }
    if (settings.performance.optimizeGcForThroughput.has_value())
    {
        resolved.optimizeGcForThroughput = *settings.performance.optimizeGcForThroughput;
    }
    if (settings.performance.pinThreadsToCores.has_value())
    {
        resolved.pinThreadsToCores = *settings.performance.pinThreadsToCores;
    }
    return normalizePerformanceSettings(resolved);
}

}  // namespace

void BeezSettings::merge(const BeezSettings& overlay)
{
    mergeOptionalValue(performance.maxThreads, overlay.performance.maxThreads);
    mergeOptionalString(performance.cacheWriteStrategy, overlay.performance.cacheWriteStrategy);
    mergeOptionalValue(performance.cacheFilesystemMetadata,
                       overlay.performance.cacheFilesystemMetadata);
    mergeOptionalValue(performance.useMmapForHashing, overlay.performance.useMmapForHashing);
    mergeOptionalValue(performance.mmapHashingMinBytes, overlay.performance.mmapHashingMinBytes);
    mergeOptionalValue(performance.optimizeGcForThroughput,
                       overlay.performance.optimizeGcForThroughput);
    mergeOptionalValue(performance.pinThreadsToCores, overlay.performance.pinThreadsToCores);
    mergeOptionalPath(cache.path, overlay.cache.path);
    mergeOptionalValue(cache.enabled, overlay.cache.enabled);
    mergeOptionalValue(cache.protect, overlay.cache.protect);
    mergeOptionalString(cache.hash.algorithm, overlay.cache.hash.algorithm);
    mergeOptionalValue(cache.hash.seed, overlay.cache.hash.seed);
    mergeOptionalString(cache.compress.algorithm, overlay.cache.compress.algorithm);
    mergeOptionalValue(cache.compress.level, overlay.cache.compress.level);
    mergeOptionalString(cache.compress.mode, overlay.cache.compress.mode);
    mergeOptionalValue(ui.outputMode, overlay.ui.outputMode);
    mergeUiSettingsOverlay(ui.options, overlay.ui.options);
    mergeEnvSettingsOverlay(env, overlay.env);
    mergeOptionalValue(dryRun, overlay.dryRun);
}

void BeezSettings::applyEnvironment(const Context& context) const
{
    applyEnvSettings(resolveEnvSettings(), context.projectRoot());
}

void BeezSettings::applyCliOverrides(const cli::ParsedOptions& options)
{
    if (options.silent)
    {
        ui.outputMode = logging::OutputMode::Silent;
    }
    else if (options.errorsOnly)
    {
        ui.outputMode = logging::OutputMode::Errors;
    }
    else if (options.verbose)
    {
        ui.outputMode = logging::OutputMode::Verbose;
    }

    if (options.dryRun)
    {
        dryRun = true;
    }

    if (!options.enableCache)
    {
        cache.enabled = false;
    }

    if (options.maxThreads.has_value())
    {
        performance.maxThreads = options.maxThreads;
    }

    if (options.noLogFile)
    {
        if (!ui.options.logging.has_value())
        {
            ui.options.logging = logging::LoggingSettingsOverlay {};
        }
        ui.options.logging->runLog = false;
    }
    else if (options.logFile.has_value())
    {
        if (!ui.options.logging.has_value())
        {
            ui.options.logging = logging::LoggingSettingsOverlay {};
        }
        ui.options.logging->runLog = true;
        ui.options.logging->runLogFile = *options.logFile;
    }
}

EnvSettings BeezSettings::resolveEnvSettings() const
{
    return ::beez::core::resolveEnvSettings(env);
}

CacheOptions BeezSettings::resolveCacheOptions(const Context& context) const
{
    CacheOptions options;
    options.enabled = cache.enabled.value_or(true);
    options.protect = cache.protect.value_or(false);
    options.hash = resolveHashSettings(cache);
    options.compress = resolveCompressionSettings(cache);
    options.envHashFingerprint = environmentHashFingerprint(resolveEnvSettings());

    const PerformanceSettings Performance = buildPerformanceSettings(*this);
    options.hash.useMmapForHashing = Performance.useMmapForHashing;
    options.hash.mmapHashingMinBytes = Performance.mmapHashingMinBytes;

    if (!cache.path.has_value() || cache.path->empty())
    {
        options.root = context.projectRoot() / ".cache";
    }
    else if (cache.path->is_absolute())
    {
        options.root = *cache.path;
    }
    else
    {
        options.root = context.projectRoot() / *cache.path;
    }

    return options;
}

PerformanceSettings BeezSettings::resolvePerformanceSettings() const
{
    return buildPerformanceSettings(*this);
}

UiSettings BeezSettings::resolveUiSettings() const
{
    return ::beez::core::resolveUiSettings(ui.options);
}

logging::LoggingSettings BeezSettings::resolveLoggingSettings(const Context& context) const
{
    return logging::resolveLoggingSettings(
        ui.options.logging.value_or(logging::LoggingSettingsOverlay {}), context.projectRoot());
}

RunOptions BeezSettings::toRunOptions(logging::ILogger* logger, const Context& context) const
{
    const CacheOptions Cache = resolveCacheOptions(context);
    return RunOptions {
        .dryRun = dryRun.value_or(false),
        .enableCache = Cache.enabled,
        .maxThreads = performance.maxThreads,
        .outputMode = ui.outputMode.value_or(logging::OutputMode::Clean),
        .logger = logger,
        .cache = Cache,
        .performance = buildPerformanceSettings(*this),
        .ui = resolveUiSettings(),
        .logging = resolveLoggingSettings(context),
    };
}

}  // namespace beez::core
