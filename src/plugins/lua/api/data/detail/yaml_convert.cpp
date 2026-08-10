#include "beez/plugin/lua/api/data/detail/yaml_convert.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] std::string csubstrToString(const ryml::csubstr value)
{
    return std::string(value.str, value.len);
}

[[nodiscard]] bool parseBool(const ryml::csubstr value, bool& output)
{
    if (value == "true")
    {
        output = true;
        return true;
    }

    if (value == "false")
    {
        output = false;
        return true;
    }

    return false;
}

[[nodiscard]] bool parseNumber(const ryml::csubstr value, sol::state_view luaState, sol::object& output)
{
    const std::string Text = csubstrToString(value);
    if (Text.find('.') != std::string::npos || Text.find('e') != std::string::npos ||
        Text.find('E') != std::string::npos)
    {
        double number = 0.0;
        const auto Result = std::from_chars(Text.data(), Text.data() + Text.size(), number);
        if (Result.ec == std::errc())
        {
            output = sol::make_object(luaState, number);
            return true;
        }
        return false;
    }

    std::int64_t integer = 0;
    const auto Result = std::from_chars(Text.data(), Text.data() + Text.size(), integer);
    if (Result.ec == std::errc())
    {
        output = sol::make_object(luaState, integer);
        return true;
    }

    return false;
}

[[nodiscard]] sol::object rymlScalarToLua(sol::state_view luaState, ryml::ConstNodeRef node)
{
    if (node.val_is_null())
    {
        return sol::lua_nil;
    }

    const ryml::csubstr Value = node.val();
    bool boolean = false;
    if (parseBool(Value, boolean))
    {
        return sol::make_object(luaState, boolean);
    }

    sol::object number;
    if (parseNumber(Value, luaState, number))
    {
        return number;
    }

    return sol::make_object(luaState, csubstrToString(Value));
}

void appendLuaValue(ryml::NodeRef node, const sol::object& value);

void luaTableToRyml(ryml::NodeRef node, const sol::table& table)
{
    if (isLuaArray(table))
    {
        node |= ryml::SEQ;
        for (std::size_t index = 1; index <= table.size(); ++index)
        {
            ryml::NodeRef child = node.append_child();
            appendLuaValue(child, table[index]);
        }
        return;
    }

    node |= ryml::MAP;
    table.for_each([&node](const sol::object& key, const sol::object& childValue)
                   {
                       ryml::NodeRef child = node.append_child();
                       child.set_key_serialized(key.as<std::string>());
                       appendLuaValue(child, childValue);
                   });
}

void appendLuaValue(ryml::NodeRef node, const sol::object& value)
{
    if (!value.valid() || value.is<sol::lua_nil_t>())
    {
        node.set_val_serialized(nullptr);
        return;
    }

    if (value.is<bool>())
    {
        node << value.as<bool>();
        return;
    }

    if (value.is<std::string>())
    {
        node << value.as<std::string>();
        return;
    }

    if (value.is<int>())
    {
        node << value.as<int>();
        return;
    }

    if (value.is<double>())
    {
        node << value.as<double>();
        return;
    }

    if (value.is<sol::table>())
    {
        luaTableToRyml(node, value.as<sol::table>());
        return;
    }

    throw std::runtime_error("beez.data: unsupported Lua value type for YAML serialization");
}

}  // namespace

sol::object rymlNodeToLua(sol::state_view luaState, ryml::ConstNodeRef node)
{
    if (node.is_map())
    {
        sol::table objectTable = luaState.create_table();
        for (ryml::ConstNodeRef child : node.children())
        {
            objectTable[csubstrToString(child.key())] = rymlNodeToLua(luaState, child);
        }
        return objectTable;
    }

    if (node.is_seq())
    {
        sol::table arrayTable = luaState.create_table();
        std::size_t index = 1;
        for (ryml::ConstNodeRef child : node.children())
        {
            arrayTable[index] = rymlNodeToLua(luaState, child);
            ++index;
        }
        return arrayTable;
    }

    return rymlScalarToLua(luaState, node);
}

std::string luaTableToYamlString(const sol::table& table)
{
    ryml::Tree tree;
    ryml::NodeRef root = tree.rootref();
    luaTableToRyml(root, table);

    std::string output;
    ryml::emitrs_yaml(tree, &output);
    return output;
}

}  // namespace beez::plugin::lua::data_detail
