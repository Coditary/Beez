#include "beez/plugin/lua/lua_settings.hpp"

#include "beez/core/config_paths.hpp"
#include "beez/core/settings.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/logging/output_mode.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

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

    throw std::runtime_error("ui.output_mode must be 'clean' or 'verbose'");
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

void readEnvironmentTable(const sol::table& table, core::BeezSettings& settings)
{
    table.for_each(
        [&settings](const sol::object& key, const sol::object& value)
        {
            if (!key.is<std::string>() || !value.is<std::string>())
            {
                throw std::runtime_error("environment entries must be string keys and values");
            }

            settings.environment[key.as<std::string>()] = value.as<std::string>();
        });
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
    core::BeezSettings overlay;

    if (const sol::object PerformanceValue = table["performance"]; PerformanceValue.valid())
    {
        if (!PerformanceValue.is<sol::table>())
        {
            throw std::runtime_error("performance must be a table");
        }

        const sol::table PerformanceTable = PerformanceValue.as<sol::table>();
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

    if (const sol::object PathsValue = table["paths"]; PathsValue.valid())
    {
        if (!PathsValue.is<sol::table>())
        {
            throw std::runtime_error("paths must be a table");
        }

        const sol::table PathsTable = PathsValue.as<sol::table>();
        readOptionalStringPath(PathsTable, "env_file", overlay.paths.envFile);
        readOptionalString(PathsTable, "build_script", overlay.paths.buildScript);
    }

    if (const sol::object EngineValue = table["engine"]; EngineValue.valid())
    {
        if (!EngineValue.is<sol::table>())
        {
            throw std::runtime_error("engine must be a table");
        }

        const sol::table EngineTable = EngineValue.as<sol::table>();
        readOptionalBool(EngineTable, "dry_run", overlay.engine.dryRun);
        readOptionalBool(EngineTable, "enable_cache", overlay.engine.enableCache);
    }

    if (const sol::object EnvironmentValue = table["environment"]; EnvironmentValue.valid())
    {
        if (!EnvironmentValue.is<sol::table>())
        {
            throw std::runtime_error("environment must be a table");
        }

        readEnvironmentTable(EnvironmentValue.as<sol::table>(), overlay);
    }

    settings.merge(overlay);
}

bool loadSettingsFromLuaFile(const std::filesystem::path& path, core::BeezSettings& settings)
{
    if (path.empty() || !std::filesystem::exists(path))
    {
        return true;
    }

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package);

    const sol::protected_function_result Result =
        lua.safe_script_file(path.string(), sol::script_pass_on_error);
    if (!Result.valid())
    {
        const sol::error Error = Result;
        throw std::runtime_error(std::string("failed to load settings from ") + path.string() +
                                 ": " + Error.what());
    }

    const sol::object Returned = Result.get<sol::object>();
    if (!Returned.valid() || Returned.get_type() == sol::type::lua_nil)
    {
        return true;
    }

    if (!Returned.is<sol::table>())
    {
        throw std::runtime_error("settings file must return a table");
    }

    mergeSettingsFromLuaTable(Returned.as<sol::table>(), settings);
    return true;
}

void tryLoadGlobalBeezSettings(core::BeezSettings& settings)
{
    const auto ConfigPath = core::globalBeezConfigPath();
    if (ConfigPath.empty())
    {
        return;
    }

    static_cast<void>(loadSettingsFromLuaFile(ConfigPath, settings));
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
