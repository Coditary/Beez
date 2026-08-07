#include "lua_step_config.hpp"

#include "beez/core/context.h"
#include "beez/core/glob_expand.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/success_cache.hpp"
#include "beez/core/worker_pool.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

sol::table shallowCopyTable(const std::shared_ptr<sol::state>& luaState, const sol::table& source)
{
    sol::table copy = luaState->create_table();
    source.for_each([&copy](const sol::object& key, const sol::object& value)
                    { copy[key] = value; });
    return copy;
}

sol::table mergeTables(const std::shared_ptr<sol::state>& luaState,
                       const sol::table& base,
                       const sol::table& overlay)
{
    sol::table merged = shallowCopyTable(luaState, base);
    overlay.for_each([&merged](const sol::object& key, const sol::object& value)
                     { merged[key] = value; });
    return merged;
}

[[nodiscard]] std::string serializeArrayTable(const sol::table& table)
{
    std::vector<std::string> items;
    table.for_each(
        [&items](const sol::object& /*key*/, const sol::object& value)
        {
            if (value.is<std::string>())
            {
                items.push_back(value.as<std::string>());
            }
        });

    std::ranges::sort(items);
    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < items.size(); ++index)
    {
        if (index > 0)
        {
            stream << ',';
        }
        stream << items.at(index);
    }
    stream << ']';
    return stream.str();
}

[[nodiscard]] std::string serializeTable(const sol::table& table)
{
    std::vector<std::string> entries;
    table.for_each(
        [&entries](const sol::object& key, const sol::object& value)
        {
            std::ostringstream stream;
            stream << key.as<std::string>() << '=';
            if (value.is<bool>())
            {
                stream << (value.as<bool>() ? "true" : "false");
            }
            else if (value.is<std::string>())
            {
                stream << value.as<std::string>();
            }
            else if (value.is<int>())
            {
                stream << value.as<int>();
            }
            else if (value.is<double>())
            {
                stream << value.as<double>();
            }
            else if (value.is<sol::table>())
            {
                stream << serializeArrayTable(value.as<sol::table>());
            }
            else
            {
                stream << "<unsupported>";
            }
            entries.push_back(stream.str());
        });

    std::ranges::sort(entries);
    std::ostringstream combined;
    for (const auto& entry : entries)
    {
        combined << entry << '\n';
    }
    return combined.str();
}

[[nodiscard]] std::vector<std::string> parseStringArray(const sol::table& table)
{
    std::vector<std::string> values;
    table.for_each(
        [&values](const sol::object& /*key*/, const sol::object& value)
        {
            if (value.is<std::string>())
            {
                values.push_back(value.as<std::string>());
            }
        });
    return values;
}

[[nodiscard]] core::WorkerSpec parseWorkerSpec(const sol::table& options)
{
    const sol::object NameValue = options["name"];
    if (!NameValue.valid() || !NameValue.is<std::string>())
    {
        throw std::runtime_error("worker spawn requires string field 'name'");
    }

    core::WorkerSpec spec;
    spec.name = NameValue.as<std::string>();

    const sol::object CommandValue = options["cmd"];
    if (!CommandValue.valid())
    {
        throw std::runtime_error("worker '" + spec.name + "' is missing required field 'cmd'");
    }

    if (CommandValue.is<std::string>())
    {
        spec.commands.push_back(CommandValue.as<std::string>());
    }
    else if (CommandValue.is<sol::table>())
    {
        CommandValue.as<sol::table>().for_each(
            [&spec](const sol::object& /*key*/, const sol::object& value)
            {
                if (value.is<std::string>())
                {
                    spec.commands.push_back(value.as<std::string>());
                }
            });
    }
    else
    {
        throw std::runtime_error("worker '" + spec.name +
                                 "' field 'cmd' must be a string or table");
    }

    const sol::object InputsValue = options["inputs"];
    if (InputsValue.valid())
    {
        if (!InputsValue.is<sol::table>())
        {
            throw std::runtime_error("worker '" + spec.name + "' field 'inputs' must be a table");
        }
        spec.inputs = parseStringArray(InputsValue.as<sol::table>());
    }

    const sol::object OutputsValue = options["outputs"];
    if (OutputsValue.valid())
    {
        if (!OutputsValue.is<sol::table>())
        {
            throw std::runtime_error("worker '" + spec.name + "' field 'outputs' must be a table");
        }
        spec.outputs = parseStringArray(OutputsValue.as<sol::table>());
    }

    return spec;
}

[[nodiscard]] core::WorkerHandle parseWorkerHandle(const sol::table& handleTable)
{
    const sol::object IdValue = handleTable["id"];
    if (!IdValue.valid())
    {
        throw std::runtime_error("invalid worker handle");
    }

    if (IdValue.is<std::size_t>())
    {
        return core::WorkerHandle {.id = IdValue.as<std::size_t>()};
    }

    if (IdValue.is<int>())
    {
        const int WorkerId = IdValue.as<int>();
        if (WorkerId < 0)
        {
            throw std::runtime_error("invalid worker handle");
        }
        return core::WorkerHandle {.id = static_cast<std::size_t>(WorkerId)};
    }

    throw std::runtime_error("invalid worker handle");
}

[[nodiscard]] core::WorkerHandle parseWorkerHandleFromObject(const sol::object& handleValue)
{
    if (handleValue.is<sol::table>())
    {
        return parseWorkerHandle(handleValue.as<sol::table>());
    }

    if (handleValue.is<std::size_t>())
    {
        return core::WorkerHandle {.id = handleValue.as<std::size_t>()};
    }

    if (handleValue.is<int>())
    {
        const int WorkerId = handleValue.as<int>();
        if (WorkerId < 0)
        {
            throw std::runtime_error("invalid worker handle");
        }
        return core::WorkerHandle {.id = static_cast<std::size_t>(WorkerId)};
    }

    throw std::runtime_error("invalid worker handle");
}

[[nodiscard]] std::vector<core::WorkerHandle> parseWorkerHandleList(const sol::table& handlesTable)
{
    std::vector<core::WorkerHandle> handles;
    std::size_t length = handlesTable.size();
    if (length == 0)
    {
        lua_State* luaState = handlesTable.lua_state();
        handlesTable.push();
        length = static_cast<std::size_t>(lua_rawlen(luaState, -1));
        lua_pop(luaState, 1);
    }
    for (std::size_t index = 1; index <= length; ++index)
    {
        const sol::object Entry = handlesTable.get<sol::object>(static_cast<int>(index));
        if (!Entry.valid() || Entry.is<sol::lua_nil_t>())
        {
            continue;
        }

        handles.push_back(parseWorkerHandleFromObject(Entry));
    }

    return handles;
}

class LuaStepConfig final : public core::StepConfig
{
  public:
    LuaStepConfig(std::shared_ptr<sol::state> luaState, std::function<sol::table()> lazyBuilder)
        : luaState_(std::move(luaState)), lazyBuilder_(std::move(lazyBuilder))
    {
    }

    [[nodiscard]] bool empty() const override
    {
        return !lazyBuilder_;
    }

    [[nodiscard]] std::string cacheFingerprint() const override
    {
        if (!lazyBuilder_)
        {
            return {};
        }
        return serializeTable(materialize());
    }

    [[nodiscard]] core::StepConfigPtr mergedWith(const core::StepConfigPtr& overlay) const override
    {
        const auto* overlayConfig = dynamic_cast<const LuaStepConfig*>(overlay.get());
        if (overlayConfig == nullptr)
        {
            return overlay;
        }

        return std::make_shared<LuaStepConfig>(
            luaState_,
            [luaState = luaState_,
             baseBuilder = lazyBuilder_,
             overlayBuilder = overlayConfig->lazyBuilder_]() -> sol::table
            {
                const sol::table BaseTable = baseBuilder();
                const sol::table OverlayTable = overlayBuilder();
                return mergeTables(luaState, BaseTable, OverlayTable);
            });
    }

    [[nodiscard]] sol::table materialize() const
    {
        if (!cachedTable_.has_value())
        {
            cachedTable_ = lazyBuilder_();
        }

        return cachedTable_.value();
    }

  private:
    std::shared_ptr<sol::state> luaState_;
    std::function<sol::table()> lazyBuilder_;
    mutable std::optional<sol::table> cachedTable_;
};

}  // namespace

core::StepConfigPtr makeLuaStepConfig(const std::shared_ptr<sol::state>& luaState,
                                      const sol::table& configTable)
{
    return std::make_shared<LuaStepConfig>(luaState,
                                           [luaState, configTable]() -> sol::table
                                           { return shallowCopyTable(luaState, configTable); });
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- step context API surface
sol::table bindStepContext(const std::shared_ptr<sol::state>& luaState,
                           const core::Context& context)
{
    sol::table stepContext = luaState->create_table();
    stepContext["project_root"] = context.projectRoot().string();
    stepContext["get_config"] = [&context]() -> sol::object
    {
        const core::StepConfigPtr StepConfig = context.getConfig();
        if (StepConfig == nullptr || StepConfig->empty())
        {
            return sol::lua_nil;
        }

        const auto* luaConfig = dynamic_cast<const LuaStepConfig*>(StepConfig.get());
        if (luaConfig == nullptr)
        {
            return sol::lua_nil;
        }

        return luaConfig->materialize();
    };

    stepContext["glob"] = [&context, luaState](const sol::table& patternsTable) -> sol::table
    {
        std::vector<std::string> patterns;
        patternsTable.for_each(
            [&patterns](const sol::object& /*key*/, const sol::object& value)
            {
                if (value.is<std::string>())
                {
                    patterns.push_back(value.as<std::string>());
                }
            });

        const std::vector<std::string> Files =
            core::expandGlobPatterns(patterns, context.projectRoot(), core::defaultGlobMatcher());

        sol::table files = luaState->create_table();
        for (std::size_t index = 0; index < Files.size(); ++index)
        {
            files.set(static_cast<int>(index + 1), Files.at(index));
        }

        return files;
    };

    stepContext.set_function(
        "spawn",
        [luaState, &context](const sol::table& /*self*/, const sol::table& options) -> sol::object
        {
            core::WorkerPool* pool = context.workerPool();
            if (pool == nullptr)
            {
                throw std::runtime_error("worker pool is not available in this step context");
            }

            const core::WorkerHandle Handle = pool->spawn(parseWorkerSpec(options));
            return sol::make_object(*luaState, static_cast<int>(Handle.id));
        });

    stepContext.set_function(
        "wait",
        [&context](const sol::table& /*self*/, const sol::object& handleValue) -> int
        {
            core::WorkerPool* pool = context.workerPool();
            if (pool == nullptr)
            {
                throw std::runtime_error("worker pool is not available in this step context");
            }

            return pool->wait(parseWorkerHandleFromObject(handleValue));
        });

    stepContext.set_function(
        "wait_all",
        [&context](const sol::table& /*self*/, const sol::object& handlesArg) -> int
        {
            core::WorkerPool* pool = context.workerPool();
            if (pool == nullptr)
            {
                throw std::runtime_error("worker pool is not available in this step context");
            }

            if (!handlesArg.valid() || handlesArg.is<sol::lua_nil_t>())
            {
                return pool->drainAll();
            }

            if (!handlesArg.is<sol::table>())
            {
                throw std::runtime_error("wait_all expects a table of worker handles");
            }

            const std::vector<core::WorkerHandle> Handles =
                parseWorkerHandleList(handlesArg.as<sol::table>());
            if (Handles.empty())
            {
                return pool->drainAll();
            }

            return pool->waitAll(Handles);
        });

    stepContext["success_cached"] = [&context](const std::string& key) -> bool
    {
        const core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return false;
        }

        return session->successCached(key);
    };

    stepContext["file_success_cached"] = [&context](const std::string& relativePath) -> bool
    {
        const core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return false;
        }

        return session->fileSuccessCached(relativePath);
    };

    stepContext["cache_success"] = [&context](const std::string& key)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            throw std::runtime_error("success cache is not available in this step context");
        }

        session->cacheSuccess(key);
    };

    stepContext["cache_file_success"] = [&context](const std::string& relativePath)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            throw std::runtime_error("success cache is not available in this step context");
        }

        session->cacheFileSuccess(relativePath);
    };

    stepContext["record_cache_miss"] = [&context](const std::string& key)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            throw std::runtime_error("success cache is not available in this step context");
        }

        session->recordCacheMiss(key);
    };

    stepContext["record_file_cache_miss"] = [&context](const std::string& relativePath)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            throw std::runtime_error("success cache is not available in this step context");
        }

        session->recordFileCacheMiss(relativePath);
    };

    stepContext["get_cache_misses"] = [&context, luaState]() -> sol::table
    {
        const core::SuccessCacheSession* session = context.successCacheSession();
        sol::table misses = luaState->create_table();
        if (session == nullptr)
        {
            return misses;
        }

        const std::vector<std::string> Values = session->getCacheMisses();
        for (std::size_t index = 0; index < Values.size(); ++index)
        {
            misses.set(static_cast<int>(index + 1), Values.at(index));
        }

        return misses;
    };

    return stepContext;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters)
