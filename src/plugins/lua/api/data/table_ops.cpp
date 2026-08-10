#include "beez/plugin/lua/api/data/table_ops.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] bool objectsEqual(const sol::object& left, const sol::object& right)
{
    if (!left.valid() && !right.valid())
    {
        return true;
    }

    if (!left.valid() || !right.valid())
    {
        return false;
    }

    if (left.get_type() != right.get_type())
    {
        return false;
    }

    if (left.is<sol::lua_nil_t>() && right.is<sol::lua_nil_t>())
    {
        return true;
    }

    if (left.is<bool>())
    {
        return left.as<bool>() == right.as<bool>();
    }

    if (left.is<std::string>())
    {
        return left.as<std::string>() == right.as<std::string>();
    }

    if (left.is<int>())
    {
        return right.is<int>() && left.as<int>() == right.as<int>();
    }

    if (left.is<double>())
    {
        return right.is<double>() && left.as<double>() == right.as<double>();
    }

    if (left.is<sol::table>() && right.is<sol::table>())
    {
        const sol::table LeftTable = left.as<sol::table>();
        const sol::table RightTable = right.as<sol::table>();
        if (LeftTable.size() != RightTable.size())
        {
            return false;
        }

        bool equal = true;
        LeftTable.for_each([&](const sol::object& key, const sol::object& value)
                           {
                               if (!equal)
                               {
                                   return;
                               }

                               const sol::object Other = RightTable[key];
                               if (!objectsEqual(value, Other))
                               {
                                   equal = false;
                               }
                           });
        return equal;
    }

    return false;
}

void collectDiff(sol::state_view luaState,
                 sol::table& diff,
                 const std::string& prefix,
                 const sol::object& left,
                 const sol::object& right)
{
    if (objectsEqual(left, right))
    {
        return;
    }

    if (left.is<sol::table>() && right.is<sol::table>())
    {
        const sol::table LeftTable = left.as<sol::table>();
        const sol::table RightTable = right.as<sol::table>();

        LeftTable.for_each([&](const sol::object& key, const sol::object& value)
                           {
                               const std::string KeyString = key.as<std::string>();
                               const std::string ChildPrefix =
                                   prefix.empty() ? KeyString : prefix + '.' + KeyString;
                               collectDiff(luaState,
                                           diff,
                                           ChildPrefix,
                                           value,
                                           RightTable[key]);
                           });

        RightTable.for_each([&](const sol::object& key, const sol::object& value)
                            {
                                const sol::object Existing = LeftTable[key];
                                if (!Existing.valid())
                                {
                                    const std::string KeyString = key.as<std::string>();
                                    const std::string ChildPrefix =
                                        prefix.empty() ? KeyString : prefix + '.' + KeyString;
                                    collectDiff(luaState, diff, ChildPrefix, sol::lua_nil, value);
                                }
                            });
        return;
    }

    sol::table entry = luaState.create_table();
    entry["from"] = left.valid() ? left : sol::lua_nil;
    entry["to"] = right.valid() ? right : sol::lua_nil;
    diff[prefix] = entry;
}

}  // namespace

std::vector<std::string> splitPath(const std::string& path)
{
    std::vector<std::string> segments;
    std::stringstream stream(path);
    std::string segment;
    while (std::getline(stream, segment, '.'))
    {
        if (segment.empty())
        {
            throw std::runtime_error("beez.data: invalid empty path segment in '" + path + "'");
        }
        segments.push_back(segment);
    }

    if (segments.empty())
    {
        throw std::runtime_error("beez.data: path must not be empty");
    }

    return segments;
}

void deepMerge(sol::table& target, const sol::table& source)
{
    sol::state_view luaState(target.lua_state());
    source.for_each([&target, &luaState](const sol::object& key, const sol::object& value)
                    {
                        const sol::object Existing = target[key];
                        if (Existing.valid() && Existing.is<sol::table>() && value.is<sol::table>())
                        {
                            sol::table nestedTarget = Existing.as<sol::table>();
                            deepMerge(nestedTarget, value.as<sol::table>());
                            return;
                        }

                        if (value.is<sol::table>())
                        {
                            target[key] = cloneTable(luaState, value.as<sol::table>());
                            return;
                        }

                        target[key] = value;
                    });
}

sol::table cloneTable(sol::state_view luaState, const sol::table& table)
{
    sol::table copy = luaState.create_table();
    table.for_each([&copy, &luaState](const sol::object& key, const sol::object& value)
                   {
                       if (value.is<sol::table>())
                       {
                           copy[key] = cloneTable(luaState, value.as<sol::table>());
                           return;
                       }

                       copy[key] = value;
                   });
    return copy;
}

sol::object getPath(const sol::table& table,
                    const std::string& path,
                    const sol::object& defaultValue)
{
    const std::vector<std::string> Segments = splitPath(path);
    sol::object current = sol::make_object(table.lua_state(), table);

    for (const std::string& segment : Segments)
    {
        if (!current.valid() || !current.is<sol::table>())
        {
            return defaultValue;
        }

        const sol::table CurrentTable = current.as<sol::table>();
        current = CurrentTable[segment];
        if (!current.valid())
        {
            return defaultValue;
        }
    }

    return current;
}

void setPath(sol::table& table, const std::string& path, const sol::object& value)
{
    const std::vector<std::string> Segments = splitPath(path);
    sol::state_view luaState(table.lua_state());
    sol::table current = table;

    for (std::size_t index = 0; index + 1 < Segments.size(); ++index)
    {
        const std::string& segment = Segments.at(index);
        sol::object child = current[segment];
        if (!child.valid() || !child.is<sol::table>())
        {
            sol::table created = luaState.create_table();
            current[segment] = created;
            current = created;
            continue;
        }

        current = child.as<sol::table>();
    }

    const std::string& finalSegment = Segments.back();
    if (value.is<sol::table>())
    {
        current[finalSegment] = cloneTable(luaState, value.as<sol::table>());
        return;
    }

    current[finalSegment] = value;
}

sol::table diffTables(sol::state_view luaState, const sol::table& left, const sol::table& right)
{
    sol::table diff = luaState.create_table();
    collectDiff(luaState,
                diff,
                "",
                sol::make_object(left.lua_state(), left),
                sol::make_object(right.lua_state(), right));
    return diff;
}

}  // namespace beez::plugin::lua::data_detail
