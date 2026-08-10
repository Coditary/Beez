#include "beez/plugin/lua/settings/settings_overlay.hpp"

#include "beez/core/config/env/env_settings.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/config/ui/types.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::vector<std::string> readStringArray(const sol::table& table);

void warnUnknownTableKeys(const sol::table& table,
                          const std::unordered_set<std::string>& knownKeys,
                          const std::string& prefix)
{
    table.for_each(
        [&knownKeys, &prefix](const sol::object& key, const sol::object& /*value*/)
        {
            if (!key.is<std::string>())
            {
                return;
            }

            const std::string KeyString = key.as<std::string>();
            if (!knownKeys.contains(KeyString))
            {
                std::cerr << "Warning: unknown beez.config key '" << prefix << KeyString
                          << "' (ignored)\n";
            }
        });
}

[[nodiscard]] std::optional<logging::OutputMode> parseOutputMode(const std::string& value)
{
    if (value == "verbose")
    {
        return logging::OutputMode::Verbose;
    }

    if (value == "clean")
    {
        return logging::OutputMode::Clean;
    }

    if (value == "errors")
    {
        return logging::OutputMode::Errors;
    }

    if (value == "silent")
    {
        return logging::OutputMode::Silent;
    }

    throw std::runtime_error("ui.output_mode must be 'clean', 'verbose', 'errors', or 'silent'");
}

template <typename T>
void readOptionalNumber(const sol::table& table, const char* key, std::optional<T>& target)
{
    const sol::object Value = table[key];
    if (!Value.valid())
    {
        return;
    }

    if (!Value.is<int>())
    {
        throw std::runtime_error(std::string(key) + " must be a number");
    }

    const int Number = Value.as<int>();
    if (Number < 0)
    {
        throw std::runtime_error(std::string(key) + " must be non-negative");
    }

    target = static_cast<T>(Number);
}

void readOptionalStringPath(const sol::table& table,
                            const char* key,
                            std::optional<std::filesystem::path>& target)
{
    const sol::object Value = table[key];
    if (!Value.valid() || Value.get_type() == sol::type::lua_nil)
    {
        return;
    }

    if (!Value.is<std::string>())
    {
        throw std::runtime_error(std::string(key) + " must be a string");
    }

    target = Value.as<std::string>();
}

void readOptionalString(const sol::table& table,
                        const char* key,
                        std::optional<std::string>& target)
{
    const sol::object Value = table[key];
    if (!Value.valid() || Value.get_type() == sol::type::lua_nil)
    {
        return;
    }

    if (!Value.is<std::string>())
    {
        throw std::runtime_error(std::string(key) + " must be a string");
    }

    target = Value.as<std::string>();
}

void readOptionalBool(const sol::table& table, const char* key, std::optional<bool>& target)
{
    const sol::object Value = table[key];
    if (!Value.valid())
    {
        return;
    }

    if (!Value.is<bool>())
    {
        throw std::runtime_error(std::string(key) + " must be a boolean");
    }

    target = Value.as<bool>();
}

void readVarsTable(const sol::table& table, std::unordered_map<std::string, std::string>& target)
{
    table.for_each(
        [&target](const sol::object& key, const sol::object& value)
        {
            if (!key.is<std::string>() || !value.is<std::string>())
            {
                throw std::runtime_error("env.vars entries must be string keys and values");
            }

            target[key.as<std::string>()] = value.as<std::string>();
        });
}

[[nodiscard]] std::vector<std::string> readStringListValue(const sol::object& value,
                                                           const char* key)
{
    if (value.is<std::string>())
    {
        return {value.as<std::string>()};
    }

    if (!value.is<sol::table>())
    {
        throw std::runtime_error(std::string(key) + " must be a string or list of strings");
    }

    return readStringArray(value.as<sol::table>());
}

void readEnvFilePaths(const sol::object& value, std::vector<std::filesystem::path>& target)
{
    if (value.is<std::string>())
    {
        target.emplace_back(value.as<std::string>());
        return;
    }

    if (!value.is<sol::table>())
    {
        throw std::runtime_error("env.files must be a string or list of strings");
    }

    const auto Paths = readStringArray(value.as<sol::table>());
    target.reserve(target.size() + Paths.size());
    std::ranges::transform(Paths,
                           std::back_inserter(target),
                           [](const std::string& path) { return std::filesystem::path(path); });
}

void readEnvSettings(const sol::table& envTable, core::EnvSettingsOverlay& env)
{
    readOptionalBool(envTable, "load_dotenv", env.loadDotenv);
    readOptionalBool(envTable, "dotenv_overrides_system", env.dotenvOverridesSystem);

    if (const sol::object FilesValue = envTable["files"]; FilesValue.valid())
    {
        readEnvFilePaths(FilesValue, env.files);
    }

    if (const sol::object VarsValue = envTable["vars"]; VarsValue.valid())
    {
        if (!VarsValue.is<sol::table>())
        {
            throw std::runtime_error("env.vars must be a table");
        }

        readVarsTable(VarsValue.as<sol::table>(), env.vars);
    }

    if (const sol::object HashVarsValue = envTable["hash_vars"]; HashVarsValue.valid())
    {
        env.hashVars = readStringListValue(HashVarsValue, "env.hash_vars");
    }

    if (const sol::object IgnoreVarsValue = envTable["ignore_vars_for_hashing"];
        IgnoreVarsValue.valid())
    {
        env.ignoreVarsForHashing =
            readStringListValue(IgnoreVarsValue, "env.ignore_vars_for_hashing");
    }

    if (const sol::object MaskSecretsValue = envTable["mask_secrets"]; MaskSecretsValue.valid())
    {
        env.maskSecrets = readStringListValue(MaskSecretsValue, "env.mask_secrets");
    }
}

void readCacheSettings(const sol::table& cacheTable, core::BeezSettings& overlay)
{
    readOptionalStringPath(cacheTable, "path", overlay.cache.path);
    readOptionalBool(cacheTable, "enabled", overlay.cache.enabled);
    readOptionalBool(cacheTable, "protect", overlay.cache.protect);

    if (const sol::object HashValue = cacheTable["hash"]; HashValue.valid())
    {
        if (!HashValue.is<sol::table>())
        {
            throw std::runtime_error("cache.hash must be a table");
        }

        const sol::table HashTable = HashValue.as<sol::table>();
        readOptionalString(HashTable, "algorithm", overlay.cache.hash.algorithm);
        readOptionalNumber(HashTable, "seed", overlay.cache.hash.seed);
    }

    if (const sol::object CompressValue = cacheTable["compress"]; CompressValue.valid())
    {
        if (!CompressValue.is<sol::table>())
        {
            throw std::runtime_error("cache.compress must be a table");
        }

        const sol::table CompressTable = CompressValue.as<sol::table>();
        readOptionalString(CompressTable, "algorithm", overlay.cache.compress.algorithm);
        readOptionalNumber(CompressTable, "level", overlay.cache.compress.level);
        readOptionalString(CompressTable, "mode", overlay.cache.compress.mode);
    }
}

void readStringField(const sol::table& table, const char* key, std::string& target)
{
    const sol::object Value = table[key];
    if (!Value.valid() || Value.get_type() == sol::type::lua_nil)
    {
        return;
    }

    if (!Value.is<std::string>())
    {
        throw std::runtime_error(std::string(key) + " must be a string");
    }

    target = Value.as<std::string>();
}

void readOptionalStringField(const sol::table& table,
                             const char* key,
                             std::optional<std::string>& target)
{
    const sol::object Value = table[key];
    if (!Value.valid() || Value.get_type() == sol::type::lua_nil)
    {
        return;
    }

    if (!Value.is<std::string>())
    {
        throw std::runtime_error(std::string(key) + " must be a string");
    }

    target = Value.as<std::string>();
}

void readColorPaletteField(const sol::table& table, const char* key, std::string& target)
{
    readStringField(table, key, target);
}

[[nodiscard]] core::UiColorPalette readColorPalette(const sol::table& table)
{
    core::UiColorPalette palette;
    readColorPaletteField(table, "text", palette.text);
    readColorPaletteField(table, "muted", palette.muted);
    readColorPaletteField(table, "success", palette.success);
    readColorPaletteField(table, "warning", palette.warning);
    readColorPaletteField(table, "error", palette.error);
    readColorPaletteField(table, "info", palette.info);
    readColorPaletteField(table, "accent", palette.accent);
    readColorPaletteField(table, "progress_fill", palette.progressFill);
    readColorPaletteField(table, "progress_empty", palette.progressEmpty);
    readColorPaletteField(table, "cache_hit", palette.cacheHit);
    readColorPaletteField(table, "worker_prefix", palette.workerPrefix);
    return palette;
}

[[nodiscard]] core::CustomProgressStyle readCustomProgressStyle(const sol::table& table)
{
    core::CustomProgressStyle style;
    readStringField(table, "start", style.startDelimiter);
    readStringField(table, "end_delimiter", style.endDelimiter);
    readStringField(table, "fill", style.fillChar);
    readStringField(table, "empty", style.emptyChar);
    return style;
}

void readLegacyNumbersIndicator(const sol::table& table, core::UiAnimationOverlay& animation)
{
    const sol::object NumbersValue = table["numbers"];
    if (!NumbersValue.valid() || animation.indicator.has_value())
    {
        return;
    }

    if (!NumbersValue.is<std::string>())
    {
        throw std::runtime_error("ui.animation.progress.numbers must be a string");
    }

    const std::string Numbers = NumbersValue.as<std::string>();
    if (Numbers == "percent")
    {
        animation.indicator = "percent";
        return;
    }

    if (Numbers == "fraction" || Numbers == "both")
    {
        animation.indicator = "step";
    }
}

[[nodiscard]] std::vector<std::string> readStringArray(const sol::table& table)
{
    std::vector<std::pair<int, std::string>> indexedValues;
    table.for_each(
        [&indexedValues](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                throw std::runtime_error(
                    "ui.animation.indicator frame list must use numeric indices");
            }

            if (!value.is<std::string>())
            {
                throw std::runtime_error("ui.animation.indicator frames must be strings");
            }

            indexedValues.emplace_back(key.as<int>(), value.as<std::string>());
        });

    std::ranges::sort(indexedValues,
                      [](const auto& left, const auto& right) { return left.first < right.first; });

    std::vector<std::string> values;
    values.reserve(indexedValues.size());
    for (const auto& [index, value] : indexedValues)
    {
        (void)index;
        values.push_back(value);
    }

    return values;
}

[[nodiscard]] bool isIndicatorConfigTable(const sol::table& table)
{
    bool hasStringKey = false;
    table.for_each(
        [&](const sol::object& key, const sol::object& /*value*/)
        {
            if (!key.is<int>())
            {
                hasStringKey = true;
            }
        });

    return hasStringKey;
}

void readIndicatorConfigTable(const sol::table& table, core::UiAnimationOverlay& animation)
{
    const sol::object TypeValue = table["type"];
    const sol::object StyleValue = table["style"];
    if (TypeValue.valid())
    {
        if (!TypeValue.is<std::string>())
        {
            throw std::runtime_error("ui.animation.indicator.type must be a string");
        }

        animation.indicator = TypeValue.as<std::string>();
    }
    else if (StyleValue.valid())
    {
        if (!StyleValue.is<std::string>())
        {
            throw std::runtime_error("ui.animation.indicator.style must be a string");
        }

        animation.indicator = StyleValue.as<std::string>();
    }

    const sol::object FramesValue = table["frames"];
    if (FramesValue.valid())
    {
        if (!FramesValue.is<sol::table>())
        {
            throw std::runtime_error("ui.animation.indicator.frames must be a table");
        }

        animation.customIndicatorFrames = readStringArray(FramesValue.as<sol::table>());
    }

    readOptionalStringField(table, "start", animation.indicatorStartDelimiter);
    readOptionalStringField(table, "end_delimiter", animation.indicatorEndDelimiter);
    readOptionalNumber(table, "spin_interval", animation.indicatorSpinIntervalMs);
}

void readIndicatorValue(const sol::object& indicatorValue,
                        core::UiAnimationOverlay& animation,
                        const char* key)
{
    if (!indicatorValue.valid())
    {
        return;
    }

    if (indicatorValue.is<std::string>())
    {
        animation.indicator = indicatorValue.as<std::string>();
        return;
    }

    if (indicatorValue.is<sol::table>())
    {
        const sol::table IndicatorTable = indicatorValue.as<sol::table>();
        if (isIndicatorConfigTable(IndicatorTable))
        {
            readIndicatorConfigTable(IndicatorTable, animation);
            return;
        }

        animation.customIndicatorFrames = readStringArray(IndicatorTable);
        return;
    }

    throw std::runtime_error(std::string(key) + " must be a string or table");
}

void readAnimationSettings(const sol::table& animationTable, core::UiAnimationOverlay& animation)
{
    const sol::object ProgressValue = animationTable["progress"];
    if (ProgressValue.valid())
    {
        if (ProgressValue.is<std::string>())
        {
            animation.progress = ProgressValue.as<std::string>();
        }
        else if (ProgressValue.is<sol::table>())
        {
            const sol::table ProgressTable = ProgressValue.as<sol::table>();
            animation.customProgress = readCustomProgressStyle(ProgressTable);
            readLegacyNumbersIndicator(ProgressTable, animation);
            readIndicatorValue(
                ProgressTable["indicator"], animation, "ui.animation.progress.indicator");
        }
        else
        {
            throw std::runtime_error("ui.animation.progress must be a string or table");
        }
    }

    readIndicatorValue(animationTable["indicator"], animation, "ui.animation.indicator");
    readOptionalNumber(
        animationTable, "indicator_spin_interval", animation.indicatorSpinIntervalMs);

    const sol::object SpinnerValue = animationTable["spinner"];
    if (SpinnerValue.valid() && !animation.indicator.has_value() &&
        !animation.customIndicatorFrames.has_value())
    {
        readIndicatorValue(SpinnerValue, animation, "ui.animation.spinner");
    }
}

void readLoggingSettings(const sol::table& loggingTable, core::BeezSettings& overlay)
{
    if (!overlay.ui.options.logging.has_value())
    {
        overlay.ui.options.logging = logging::LoggingSettingsOverlay {};
    }

    logging::LoggingSettingsOverlay& logging = *overlay.ui.options.logging;
    readOptionalBool(loggingTable, "run_log", logging.runLog);
    readOptionalStringPath(loggingTable, "run_log_file", logging.runLogFile);
    readOptionalBool(loggingTable, "log_steps", logging.logSteps);
    readOptionalString(loggingTable, "workers", logging.workers);
    readOptionalStringPath(loggingTable, "workers_dir", logging.workersDir);
}

void readUiSettings(const sol::table& uiTable, core::BeezSettings& overlay)
{
    readOptionalBool(uiTable, "colors", overlay.ui.options.colors);
    readOptionalBool(uiTable, "truecolor", overlay.ui.options.truecolor);
    readOptionalBool(uiTable, "icons", overlay.ui.options.icons);
    readOptionalString(uiTable, "log_level", overlay.ui.options.logLevel);
    readOptionalBool(uiTable, "hide_cache_hits", overlay.ui.options.hideCacheHits);
    readOptionalBool(uiTable, "prefix", overlay.ui.options.workerPrefix);
    readOptionalString(uiTable, "prefix_format", overlay.ui.options.workerPrefixFormat);
    readOptionalBool(uiTable, "show_time_saved", overlay.ui.options.showTimeSaved);
    readOptionalString(uiTable, "summary", overlay.ui.options.summary);

    if (const sol::object LoggingValue = uiTable["logging"]; LoggingValue.valid())
    {
        if (!LoggingValue.is<sol::table>())
        {
            throw std::runtime_error("ui.logging must be a table");
        }

        readLoggingSettings(LoggingValue.as<sol::table>(), overlay);
    }

    if (const sol::object ThemesValue = uiTable["themes"]; ThemesValue.valid())
    {
        if (!ThemesValue.is<sol::table>())
        {
            throw std::runtime_error("ui.themes must be a table");
        }

        std::map<std::string, core::UiColorPalette> themes;
        ThemesValue.as<sol::table>().for_each(
            [&themes](const sol::object& key, const sol::object& value)
            {
                if (!key.is<std::string>() || !value.is<sol::table>())
                {
                    throw std::runtime_error(
                        "ui.themes entries must use string keys and table values");
                }

                themes.emplace(key.as<std::string>(), readColorPalette(value.as<sol::table>()));
            });
        overlay.ui.options.themes = std::move(themes);
    }

    readOptionalString(uiTable, "theme", overlay.ui.options.theme);

    if (const sol::object AnimationValue = uiTable["animation"]; AnimationValue.valid())
    {
        if (!AnimationValue.is<sol::table>())
        {
            throw std::runtime_error("ui.animation must be a table");
        }

        core::UiAnimationOverlay animation;
        readAnimationSettings(AnimationValue.as<sol::table>(), animation);
        overlay.ui.options.animation = std::move(animation);
    }
}

}  // namespace

void mergeSettingsFromLuaTable(const sol::table& table, core::BeezSettings& settings)
{
    warnUnknownTableKeys(table, {"performance", "cache", "ui", "env"}, "");

    core::BeezSettings overlay;

    if (const sol::object PerformanceValue = table["performance"]; PerformanceValue.valid())
    {
        if (!PerformanceValue.is<sol::table>())
        {
            throw std::runtime_error("performance must be a table");
        }

        const sol::table PerformanceTable = PerformanceValue.as<sol::table>();
        warnUnknownTableKeys(PerformanceTable,
                             {"max_threads",
                              "cache_write_strategy",
                              "cache_fs_metadata",
                              "use_mmap_for_hashing",
                              "mmap_hashing_min_bytes",
                              "optimize_gc_for_throughput",
                              "pin_threads_to_cores"},
                             "performance.");
        readOptionalNumber(PerformanceTable, "max_threads", overlay.performance.maxThreads);
        readOptionalString(
            PerformanceTable, "cache_write_strategy", overlay.performance.cacheWriteStrategy);
        readOptionalBool(
            PerformanceTable, "cache_fs_metadata", overlay.performance.cacheFilesystemMetadata);
        readOptionalBool(
            PerformanceTable, "use_mmap_for_hashing", overlay.performance.useMmapForHashing);
        readOptionalNumber(
            PerformanceTable, "mmap_hashing_min_bytes", overlay.performance.mmapHashingMinBytes);
        readOptionalBool(PerformanceTable,
                         "optimize_gc_for_throughput",
                         overlay.performance.optimizeGcForThroughput);
        readOptionalBool(
            PerformanceTable, "pin_threads_to_cores", overlay.performance.pinThreadsToCores);
    }

    if (const sol::object CacheValue = table["cache"]; CacheValue.valid())
    {
        if (!CacheValue.is<sol::table>())
        {
            throw std::runtime_error("cache must be a table");
        }

        const sol::table CacheTable = CacheValue.as<sol::table>();
        warnUnknownTableKeys(
            CacheTable, {"path", "enabled", "protect", "hash", "compress"}, "cache.");
        readCacheSettings(CacheTable, overlay);
    }

    if (const sol::object UiValue = table["ui"]; UiValue.valid())
    {
        if (!UiValue.is<sol::table>())
        {
            throw std::runtime_error("ui must be a table");
        }

        const sol::table UiTable = UiValue.as<sol::table>();
        const sol::object OutputModeValue = UiTable["output_mode"];
        if (OutputModeValue.valid())
        {
            if (!OutputModeValue.is<std::string>())
            {
                throw std::runtime_error("ui.output_mode must be a string");
            }

            overlay.ui.outputMode = parseOutputMode(OutputModeValue.as<std::string>());
        }

        readUiSettings(UiTable, overlay);
    }

    if (const sol::object EnvValue = table["env"]; EnvValue.valid())
    {
        if (!EnvValue.is<sol::table>())
        {
            throw std::runtime_error("env must be a table");
        }

        const sol::table EnvTable = EnvValue.as<sol::table>();
        readEnvSettings(EnvTable, overlay.env);
        warnUnknownTableKeys(EnvTable,
                             {"load_dotenv",
                              "dotenv_overrides_system",
                              "files",
                              "vars",
                              "hash_vars",
                              "ignore_vars_for_hashing",
                              "mask_secrets"},
                             "env.");
    }

    settings.merge(overlay);
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
