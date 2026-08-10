#include "beez/plugin/lua/runtime/worker_parser.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

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

[[nodiscard]] bool readWaitOptionFlag(const sol::table& options, const char* key)
{
    const sol::object Value = options[key];
    return Value.valid() && Value.is<bool>() && Value.as<bool>();
}

}  // namespace

core::WorkerSpec parseWorkerSpec(const sol::table& options)
{
    core::WorkerSpec spec;

    const sol::object NameValue = options["name"];
    if (NameValue.valid())
    {
        if (!NameValue.is<std::string>())
        {
            throw std::runtime_error("worker spawn field 'name' must be a string");
        }
        spec.name = NameValue.as<std::string>();
    }

    const sol::object CommandValue = options["cmd"];
    if (!CommandValue.valid())
    {
        throw std::runtime_error("worker spawn is missing required field 'cmd'");
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
        throw std::runtime_error("worker spawn field 'cmd' must be a string or table");
    }

    const sol::object InputsValue = options["inputs"];
    if (InputsValue.valid())
    {
        if (!InputsValue.is<sol::table>())
        {
            throw std::runtime_error("worker spawn field 'inputs' must be a table");
        }
        spec.inputs = parseStringArray(InputsValue.as<sol::table>());
    }

    const sol::object OutputsValue = options["outputs"];
    if (OutputsValue.valid())
    {
        if (!OutputsValue.is<sol::table>())
        {
            throw std::runtime_error("worker spawn field 'outputs' must be a table");
        }
        spec.outputs = parseStringArray(OutputsValue.as<sol::table>());
    }

    return spec;
}

WorkerWaitOptions parseWorkerWaitOptions(const sol::table& options)
{
    static const std::unordered_set<std::string> AllowedKeys = {"exitCode",
                                                                "output",
                                                                "duration",
                                                                "cached",
                                                                "name",
                                                                "id",
                                                                "dryRun"};

    WorkerWaitOptions waitOptions;
    options.for_each(
        [&waitOptions](const sol::object& key, const sol::object& value)
        {
            if (!key.is<std::string>())
            {
                throw std::runtime_error("wait options must use string keys");
            }

            const std::string Key = key.as<std::string>();
            if (!AllowedKeys.contains(Key))
            {
                throw std::runtime_error("unknown wait option '" + Key + "'");
            }

            if (!value.is<bool>())
            {
                throw std::runtime_error("wait option '" + Key + "' must be a boolean");
            }
        });

    waitOptions.exitCode = readWaitOptionFlag(options, "exitCode");
    waitOptions.output = readWaitOptionFlag(options, "output");
    waitOptions.duration = readWaitOptionFlag(options, "duration");
    waitOptions.cached = readWaitOptionFlag(options, "cached");
    waitOptions.name = readWaitOptionFlag(options, "name");
    waitOptions.id = readWaitOptionFlag(options, "id");
    waitOptions.dryRun = readWaitOptionFlag(options, "dryRun");
    return waitOptions;
}

core::WorkerHandle parseWorkerHandleFromObject(const sol::object& handleValue)
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

std::vector<core::WorkerHandle> parseWorkerHandleList(const sol::table& handlesTable)
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

sol::object buildWorkerWaitResult(const std::shared_ptr<sol::state>& luaState,
                                  const core::WorkerSnapshot& snapshot,
                                  const WorkerWaitOptions& options)
{
    if (!options.wantsResult())
    {
        return sol::lua_nil;
    }

    sol::table result = luaState->create_table();
    if (options.exitCode)
    {
        result["exitCode"] = snapshot.exitCode;
    }
    if (options.output)
    {
        result["output"] = snapshot.output;
    }
    if (options.duration)
    {
        result["duration"] = snapshot.durationSeconds;
    }
    if (options.cached)
    {
        result["cached"] = snapshot.cached;
    }
    if (options.name)
    {
        result["name"] = snapshot.name;
    }
    if (options.id)
    {
        result["id"] = static_cast<int>(snapshot.id);
    }
    if (options.dryRun)
    {
        result["dryRun"] = snapshot.dryRun;
    }

    return sol::make_object(*luaState, result);
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
