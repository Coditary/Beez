#include "beez/plugin/lua/api/data/detail/yaml_convert.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <charconv>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,misc-no-recursion,performance-unnecessary-value-param,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-identifier-naming,modernize-return-braced-init-list,misc-const-correctness)
#include <ryml_std.hpp>
#include <sol/sol.hpp>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] std::string csubstrToString(const ryml::csubstr Value)
{
    return {Value.str, Value.len};
}

[[nodiscard]] bool parseBool(const ryml::csubstr Value, bool& output)
{
    if (Value == "true")
    {
        output = true;
        return true;
    }

    if (Value == "false")
    {
        output = false;
        return true;
    }

    return false;
}

[[nodiscard]] bool parseDouble(const std::string& text, double& output)
{
#if defined(__APPLE__)
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (errno == ERANGE || end == text.c_str() ||
        static_cast<std::size_t>(end - text.c_str()) != text.size())
    {
        return false;
    }
    output = value;
    return true;
#else
    const auto result = std::from_chars(text.data(), text.data() + text.size(), output);
    return result.ec == std::errc();
#endif
}

[[nodiscard]] bool
parseNumber(const ryml::csubstr Value, sol::state_view luaState, sol::object& output)
{
    const std::string Text = csubstrToString(Value);
    if (Text.find('.') != std::string::npos || Text.find('e') != std::string::npos ||
        Text.find('E') != std::string::npos)
    {
        double number = 0.0;
        if (!parseDouble(Text, number))
        {
            return false;
        }
        output = sol::make_object(luaState, number);
        return true;
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
            const ryml::NodeRef child = node.append_child();
            appendLuaValue(child, table[index]);
        }
        return;
    }

    node |= ryml::MAP;
    table.for_each(
        [&node](const sol::object& key, const sol::object& childValue)
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
        for (const ryml::ConstNodeRef child : node.children())
        {
            objectTable[csubstrToString(child.key())] = rymlNodeToLua(luaState, child);
        }
        return objectTable;
    }

    if (node.is_seq())
    {
        sol::table arrayTable = luaState.create_table();
        std::size_t index = 1;
        for (const ryml::ConstNodeRef child : node.children())
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
    const ryml::NodeRef root = tree.rootref();
    luaTableToRyml(root, table);

    std::string output;
    ryml::emitrs_yaml(tree, &output);
    return output;
}

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,misc-no-recursion,performance-unnecessary-value-param,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-identifier-naming,modernize-return-braced-init-list,misc-const-correctness)
