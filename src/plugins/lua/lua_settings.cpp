#include "beez/plugin/lua/lua_settings.hpp"

#include "beez/core/config_paths.hpp"
#include "beez/core/settings.hpp"
#include "beez/logging/output_mode.hpp"

#include <filesystem>
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
