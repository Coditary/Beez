#include "beez/plugin/lua/runtime/worker_parser.hpp"

#include <stdexcept>
#include <string>
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

}  // namespace

core::WorkerSpec parseWorkerSpec(const sol::table& options)
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

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
