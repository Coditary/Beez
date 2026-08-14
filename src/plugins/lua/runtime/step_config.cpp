#include "beez/plugin/lua/runtime/step_config.hpp"

#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/execution/concurrency/worker_pool.hpp"
#include "beez/plugin/lua/api/fs/detail/glob.hpp"
#include "beez/plugin/lua/runtime/worker_parser.hpp"

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

sol::object copyLuaObject(const std::shared_ptr<sol::state>& luaState, const sol::object& value)
{
    if (value.is<bool>())
    {
        return sol::make_object(*luaState, value.as<bool>());
    }

    if (value.is<int>())
    {
        return sol::make_object(*luaState, value.as<int>());
    }

    if (value.is<double>())
    {
        return sol::make_object(*luaState, value.as<double>());
    }

    if (value.is<std::string>())
    {
        return sol::make_object(*luaState, value.as<std::string>());
    }

    if (value.is<sol::table>())
    {
        sol::table copy = luaState->create_table();
        value.as<sol::table>().for_each(
            [&copy, luaState](const sol::object& key, const sol::object& entry)
            { copy[copyLuaObject(luaState, key)] = copyLuaObject(luaState, entry); });
        return sol::make_object(*luaState, copy);
    }

    return sol::lua_nil;
}

sol::table shallowCopyTable(const std::shared_ptr<sol::state>& luaState, const sol::table& source)
{
    sol::table copy = luaState->create_table();
    source.for_each([&copy, luaState](const sol::object& key, const sol::object& value)
                    { copy[copyLuaObject(luaState, key)] = copyLuaObject(luaState, value); });
    return copy;
}

sol::table mergeTables(const std::shared_ptr<sol::state>& luaState,
                       const sol::table& base,
                       const sol::table& overlay)
{
    sol::table merged = shallowCopyTable(luaState, base);
    overlay.for_each([&merged, luaState](const sol::object& key, const sol::object& value)
                     { merged[copyLuaObject(luaState, key)] = copyLuaObject(luaState, value); });
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
    stepContext["verbose"] = [&context]() -> bool { return context.verboseOutput(); };
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

        return fs_detail::globPatternsToTable(
            luaState, patterns, context.projectRoot(), context.globMetadataCache());
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
        [luaState, &context](const sol::table& /*self*/,
                             const sol::object& handleValue,
                             sol::optional<sol::table> optionsTable) -> sol::object
        {
            core::WorkerPool* pool = context.workerPool();
            if (pool == nullptr)
            {
                throw std::runtime_error("worker pool is not available in this step context");
            }

            const core::WorkerHandle Handle = parseWorkerHandleFromObject(handleValue);
            const int ExitCode = pool->wait(Handle);
            if (ExitCode == 0)
            {
                context.setPendingWorkerDuration(pool->workerDuration(Handle.id));
            }

            if (!optionsTable.has_value())
            {
                return sol::lua_nil;
            }

            const WorkerWaitOptions Options = parseWorkerWaitOptions(optionsTable.value());
            return buildWorkerWaitResult(luaState, pool->workerSnapshot(Handle.id), Options);
        });

    stepContext.set_function(
        "wait_all",
        [luaState, &context](const sol::table& /*self*/,
                             sol::optional<sol::object> handlesArg,
                             sol::optional<sol::table> optionsTable) -> sol::object
        {
            core::WorkerPool* pool = context.workerPool();
            if (pool == nullptr)
            {
                throw std::runtime_error("worker pool is not available in this step context");
            }

            std::vector<core::WorkerHandle> handles;
            if (!handlesArg.has_value() || !handlesArg->valid() || handlesArg->is<sol::lua_nil_t>())
            {
                (void)pool->drainAll();
                for (std::size_t index = 0; index < pool->workerCount(); ++index)
                {
                    handles.push_back(core::WorkerHandle {.id = index});
                }
            }
            else if (handlesArg->is<sol::table>())
            {
                handles = parseWorkerHandleList(handlesArg->as<sol::table>());
                if (handles.empty())
                {
                    (void)pool->drainAll();
                    for (std::size_t index = 0; index < pool->workerCount(); ++index)
                    {
                        handles.push_back(core::WorkerHandle {.id = index});
                    }
                }
                else
                {
                    (void)pool->waitAll(handles);
                }
            }
            else
            {
                throw std::runtime_error("wait_all expects a table of worker handles");
            }

            if (!optionsTable.has_value())
            {
                return sol::lua_nil;
            }

            const WorkerWaitOptions Options = parseWorkerWaitOptions(optionsTable.value());
            if (!Options.wantsResult())
            {
                return sol::lua_nil;
            }

            sol::table results = luaState->create_table();
            for (std::size_t index = 0; index < handles.size(); ++index)
            {
                const core::WorkerHandle& handle = handles.at(index);
                const sol::object Entry =
                    buildWorkerWaitResult(luaState, pool->workerSnapshot(handle.id), Options);
                results.set(static_cast<int>(index + 1), Entry);
            }

            return sol::make_object(*luaState, results);
        });

    stepContext["success_cached"] = [&context](const std::string& key) -> bool
    {
        const core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return false;
        }

        const bool Cached = session->successCached(key);
        if (Cached)
        {
            context.recordCacheUnit(true, 0.0);
        }

        return Cached;
    };

    stepContext["file_success_cached"] = [&context](const std::string& relativePath) -> bool
    {
        const core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return false;
        }

        const bool Cached = session->fileSuccessCached(relativePath);
        if (Cached)
        {
            context.recordCacheUnit(true, session->fileSavedDurationSeconds(relativePath));
        }

        return Cached;
    };

    stepContext["cache_success"] = [&context](const std::string& key)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return;
        }

        session->cacheSuccess(key);
    };

    stepContext["cache_file_success"] =
        [&context](const std::string& relativePath, sol::optional<double> durationSeconds)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return;
        }

        const double Duration = durationSeconds.value_or(context.consumePendingWorkerDuration());
        session->cacheFileSuccess(relativePath, Duration);
    };

    stepContext["record_cache_miss"] = [&context](const std::string& key)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return;
        }

        session->recordCacheMiss(key);
    };

    stepContext["record_file_cache_miss"] = [&context](const std::string& relativePath)
    {
        core::SuccessCacheSession* session = context.successCacheSession();
        if (session == nullptr)
        {
            return;
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
