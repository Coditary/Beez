#include "beez/core/settings.hpp"

#include "beez/cli/parsed_options.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/run_options.hpp"
#include "beez/logging/output_mode.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

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

}  // namespace

void BeezSettings::merge(const BeezSettings& overlay)
{
    mergeOptionalValue(performance.maxThreads, overlay.performance.maxThreads);
    mergeOptionalPath(cache.path, overlay.cache.path);
    mergeOptionalValue(cache.enabled, overlay.cache.enabled);
    mergeOptionalValue(cache.protect, overlay.cache.protect);
    mergeOptionalString(cache.hash.algorithm, overlay.cache.hash.algorithm);
    mergeOptionalValue(cache.hash.seed, overlay.cache.hash.seed);
    mergeOptionalString(cache.compress.algorithm, overlay.cache.compress.algorithm);
    mergeOptionalValue(cache.compress.level, overlay.cache.compress.level);
    mergeOptionalString(cache.compress.mode, overlay.cache.compress.mode);
    mergeOptionalValue(ui.outputMode, overlay.ui.outputMode);
    mergeOptionalPath(paths.envFile, overlay.paths.envFile);
    mergeOptionalString(paths.buildScript, overlay.paths.buildScript);
    mergeOptionalValue(engine.dryRun, overlay.engine.dryRun);
    mergeOptionalValue(engine.enableCache, overlay.engine.enableCache);

    for (const auto& [key, value] : overlay.environment)
    {
        environment[key] = value;
    }
}

void BeezSettings::applyEnvironment() const
{
    for (const auto& [key, value] : environment)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,misc-include-cleaner)
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void BeezSettings::applyCliOverrides(const cli::ParsedOptions& options)
{
    if (options.verbose)
    {
        ui.outputMode = logging::OutputMode::Verbose;
    }

    if (options.dryRun)
    {
        engine.dryRun = true;
    }

    if (!options.enableCache)
    {
        cache.enabled = false;
        engine.enableCache = false;
    }

    if (options.maxThreads.has_value())
    {
        performance.maxThreads = options.maxThreads;
    }
}

void BeezSettings::applyToContext(Context& context) const
{
    if (paths.buildScript.has_value())
    {
        context.setBuildScriptFileName(*paths.buildScript);
    }

    if (paths.envFile.has_value())
    {
        context.setEnvFilePath(*paths.envFile);
    }
}

CacheOptions BeezSettings::resolveCacheOptions(const Context& context) const
{
    CacheOptions options;
    options.enabled = cache.enabled.value_or(engine.enableCache.value_or(true));
    options.protect = cache.protect.value_or(false);
    options.hash = resolveHashSettings(cache);
    options.compress = resolveCompressionSettings(cache);

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

RunOptions BeezSettings::toRunOptions(logging::ILogger* logger, const Context& context) const
{
    const CacheOptions Cache = resolveCacheOptions(context);
    return RunOptions {
        .dryRun = engine.dryRun.value_or(false),
        .enableCache = Cache.enabled,
        .maxThreads = performance.maxThreads,
        .outputMode = ui.outputMode.value_or(logging::OutputMode::Clean),
        .logger = logger,
        .cache = Cache,
    };
}

}  // namespace beez::core
