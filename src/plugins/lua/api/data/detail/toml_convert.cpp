#include "beez/plugin/lua/api/data/detail/toml_convert.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

#include <toml++/toml.hpp>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] sol::object tomlNodeToLua(sol::state_view luaState, const toml::node& node);
[[nodiscard]] toml::table luaTableToTomlTable(const sol::table& table);
[[nodiscard]] toml::array luaTableToTomlArray(const sol::table& table);

[[nodiscard]] sol::object tomlNodeToLua(sol::state_view luaState, const toml::node& node)
{
    if (node.is_table())
    {
        sol::table objectTable = luaState.create_table();
        for (const auto& [key, value] : *node.as_table())
        {
            objectTable[std::string(key.str())] = tomlNodeToLua(luaState, value);
        }
        return objectTable;
    }

    if (node.is_array())
    {
        sol::table arrayTable = luaState.create_table();
        std::size_t index = 1;
        for (const auto& value : *node.as_array())
        {
            arrayTable[index] = tomlNodeToLua(luaState, value);
            ++index;
        }
        return arrayTable;
    }

    if (node.is_string())
    {
        const std::string_view Value = *node.value<std::string_view>();
        return sol::make_object(luaState, std::string(Value));
    }

    if (node.is_integer())
    {
        return sol::make_object(luaState, node.value<std::int64_t>());
    }

    if (node.is_floating_point())
    {
        return sol::make_object(luaState, node.value<double>());
    }

    if (node.is_boolean())
    {
        return sol::make_object(luaState, node.value<bool>());
    }

    if (node.is_date() || node.is_time() || node.is_date_time())
    {
        std::stringstream stream;
        stream << toml::default_formatter(node);
        return sol::make_object(luaState, stream.str());
    }

    return sol::lua_nil;
}

[[nodiscard]] toml::array luaTableToTomlArray(const sol::table& table)
{
    toml::array array;
    for (std::size_t index = 1; index <= table.size(); ++index)
    {
        const sol::object Value = table[index];
        if (!Value.valid() || Value.is<sol::lua_nil_t>())
        {
            throw std::runtime_error("beez.data: TOML arrays cannot contain nil values");
        }

        if (Value.is<bool>())
        {
            array.push_back(Value.as<bool>());
            continue;
        }

        if (Value.is<std::string>())
        {
            array.push_back(Value.as<std::string>());
            continue;
        }

        if (Value.is<int>())
        {
            array.push_back(Value.as<int>());
            continue;
        }

        if (Value.is<double>())
        {
            array.push_back(Value.as<double>());
            continue;
        }

        if (Value.is<sol::table>())
        {
            const sol::table Nested = Value.as<sol::table>();
            if (isLuaArray(Nested))
            {
                array.push_back(luaTableToTomlArray(Nested));
            }
            else
            {
                array.push_back(luaTableToTomlTable(Nested));
            }
            continue;
        }

        throw std::runtime_error("beez.data: unsupported Lua value type for TOML serialization");
    }

    return array;
}

[[nodiscard]] toml::table luaTableToTomlTable(const sol::table& table)
{
    toml::table result;
    table.for_each([&result](const sol::object& key, const sol::object& value)
                   {
                       if (!value.valid() || value.is<sol::lua_nil_t>())
                       {
                           return;
                       }

                       const std::string Key = key.as<std::string>();
                       if (value.is<bool>())
                       {
                           result.insert(Key, value.as<bool>());
                           return;
                       }

                       if (value.is<std::string>())
                       {
                           result.insert(Key, value.as<std::string>());
                           return;
                       }

                       if (value.is<int>())
                       {
                           result.insert(Key, value.as<int>());
                           return;
                       }

                       if (value.is<double>())
                       {
                           result.insert(Key, value.as<double>());
                           return;
                       }

                       if (value.is<sol::table>())
                       {
                           const sol::table Nested = value.as<sol::table>();
                           if (isLuaArray(Nested))
                           {
                               result.insert(Key, luaTableToTomlArray(Nested));
                           }
                           else
                           {
                               result.insert(Key, luaTableToTomlTable(Nested));
                           }
                           return;
                       }

                       throw std::runtime_error(
                           "beez.data: unsupported Lua value type for TOML serialization");
                   });
    return result;
}

}  // namespace

sol::table tomlStringToLua(sol::state_view luaState, const std::string& content)
{
    const toml::table Result = toml::parse(content);
    return tomlNodeToLua(luaState, Result).as<sol::table>();
}

std::string luaTableToTomlString(const sol::table& table)
{
    const toml::table Root = luaTableToTomlTable(table);
    std::stringstream stream;
    stream << toml::toml_formatter(Root);
    return stream.str();
}

}  // namespace beez::plugin::lua::data_detail
