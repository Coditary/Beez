#pragma once

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/env_settings.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/run_options.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/logging/logging_settings.hpp"
#include "beez/logging/output_mode.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace beez::logging
{
class ILogger;
}  // namespace beez::logging

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes) -- configuration aggregate
struct BeezSettings
{
    struct Performance
    {
        std::optional<std::size_t> maxThreads;
        std::optional<std::string> cacheWriteStrategy;
        std::optional<bool> cacheFilesystemMetadata;
        std::optional<bool> useMmapForHashing;
        std::optional<std::size_t> mmapHashingMinBytes;
        std::optional<bool> optimizeGcForThroughput;
        std::optional<bool> pinThreadsToCores;
    } performance;

    struct CacheHash
    {
        std::optional<std::string> algorithm;
        std::optional<std::uint32_t> seed;
    };

    struct CacheCompress
    {
        std::optional<std::string> algorithm;
        std::optional<int> level;
        std::optional<std::string> mode;
    };

    struct Cache
    {
        std::optional<std::filesystem::path> path;
        std::optional<bool> enabled;
        std::optional<bool> protect;
        CacheHash hash;
        CacheCompress compress;
    } cache;

    struct Ui
    {
        std::optional<logging::OutputMode> outputMode;
        UiSettingsOverlay options;
    } ui;

    EnvSettingsOverlay env;

    std::optional<bool> dryRun;

    void merge(const BeezSettings& overlay);

    void applyEnvironment(const Context& context) const;

    void applyCliOverrides(const cli::ParsedOptions& options);

    [[nodiscard]] EnvSettings resolveEnvSettings() const;

    [[nodiscard]] RunOptions toRunOptions(logging::ILogger* logger, const Context& context) const;

    [[nodiscard]] CacheOptions resolveCacheOptions(const Context& context) const;

    [[nodiscard]] PerformanceSettings resolvePerformanceSettings() const;

    [[nodiscard]] UiSettings resolveUiSettings() const;

    [[nodiscard]] logging::LoggingSettings resolveLoggingSettings(const Context& context) const;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
