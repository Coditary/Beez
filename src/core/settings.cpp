#include "beez/core/settings.hpp"

#include "beez/cli/parsed_options.hpp"
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

}  // namespace

void BeezSettings::merge(const BeezSettings& overlay)
{
    mergeOptionalValue(performance.maxThreads, overlay.performance.maxThreads);
    mergeOptionalPath(cache.directory, overlay.cache.directory);
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

RunOptions BeezSettings::toRunOptions(logging::ILogger* logger, const Context& context) const
{
    return RunOptions {
        .dryRun = engine.dryRun.value_or(false),
        .enableCache = engine.enableCache.value_or(true),
        .maxThreads = performance.maxThreads,
        .outputMode = ui.outputMode.value_or(logging::OutputMode::Clean),
        .logger = logger,
        .cacheRoot = resolveCacheDirectory(context),
    };
}

std::filesystem::path BeezSettings::resolveCacheDirectory(const Context& context) const
{
    if (!cache.directory.has_value() || cache.directory->empty())
    {
        return context.projectRoot() / ".cache";
    }

    if (cache.directory->is_absolute())
    {
        return *cache.directory;
    }

    return context.projectRoot() / *cache.directory;
}

}  // namespace beez::core
