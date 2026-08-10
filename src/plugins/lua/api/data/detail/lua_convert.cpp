#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] bool isWholeNumber(const double value)
{
    return std::floor(value) == value;
}

[[nodiscard]] yyjson_mut_val* luaObjectToYyjson(yyjson_mut_doc* document, const sol::object& object)
{
    if (!object.valid() || object.is<sol::lua_nil_t>())
    {
        return yyjson_mut_null(document);
    }

    if (object.is<bool>())
    {
        return yyjson_mut_bool(document, object.as<bool>());
    }

    if (object.is<std::string>())
    {
        const std::string Value = object.as<std::string>();
        return yyjson_mut_strncpy(document, Value.c_str(), Value.size());
    }

    if (object.is<int>())
    {
        return yyjson_mut_sint(document, object.as<int>());
    }

    if (object.is<double>())
    {
        const double Value = object.as<double>();
        if (isWholeNumber(Value))
        {
            return yyjson_mut_sint(document, static_cast<std::int64_t>(Value));
        }
        return yyjson_mut_real(document, Value);
    }

    if (object.is<sol::table>())
    {
        const sol::table Table = object.as<sol::table>();
        if (isLuaArray(Table))
        {
            yyjson_mut_val* array = yyjson_mut_arr(document);
            for (std::size_t index = 1; index <= Table.size(); ++index)
            {
                yyjson_mut_arr_append(array, luaObjectToYyjson(document, Table[index]));
            }
            return array;
        }

        yyjson_mut_val* map = yyjson_mut_obj(document);
        Table.for_each([&document, &map](const sol::object& key, const sol::object& value)
                       {
                           const std::string Key = key.as<std::string>();
                           yyjson_mut_obj_add(map,
                                              yyjson_mut_strncpy(document, Key.c_str(), Key.size()),
                                              luaObjectToYyjson(document, value));
                       });
        return map;
    }

    throw std::runtime_error("beez.data: unsupported Lua value type for serialization");
}

[[nodiscard]] sol::object yyjsonImmutableValueToLua(sol::state_view luaState, yyjson_val* value)
{
    if (value == nullptr || yyjson_is_null(value))
    {
        return sol::lua_nil;
    }

    if (yyjson_is_bool(value))
    {
        return sol::make_object(luaState, yyjson_get_bool(value));
    }

    if (yyjson_is_str(value))
    {
        return sol::make_object(luaState,
                                std::string(yyjson_get_str(value), yyjson_get_len(value)));
    }

    if (yyjson_is_int(value))
    {
        return sol::make_object(luaState, yyjson_get_sint(value));
    }

    if (yyjson_is_uint(value))
    {
        return sol::make_object(luaState, yyjson_get_uint(value));
    }

    if (yyjson_is_real(value))
    {
        return sol::make_object(luaState, yyjson_get_real(value));
    }

    if (yyjson_is_arr(value))
    {
        sol::table arrayTable = luaState.create_table();
        std::size_t index = 1;
        yyjson_val* item = nullptr;
        yyjson_arr_iter iterator = {};
        yyjson_arr_iter_init(value, &iterator);
        while ((item = yyjson_arr_iter_next(&iterator)) != nullptr)
        {
            arrayTable[index] = yyjsonImmutableValueToLua(luaState, item);
            ++index;
        }
        return arrayTable;
    }

    if (yyjson_is_obj(value))
    {
        sol::table objectTable = luaState.create_table();
        yyjson_val* key = nullptr;
        yyjson_val* item = nullptr;
        yyjson_obj_iter iterator = {};
        yyjson_obj_iter_init(value, &iterator);
        while ((key = yyjson_obj_iter_next(&iterator)) != nullptr)
        {
            item = yyjson_obj_iter_get_val(key);
            objectTable[std::string(yyjson_get_str(key), yyjson_get_len(key))] =
                yyjsonImmutableValueToLua(luaState, item);
        }
        return objectTable;
    }

    throw std::runtime_error("beez.data: unsupported JSON value type for deserialization");
}

[[nodiscard]] sol::object yyjsonMutableValueToLua(sol::state_view luaState, yyjson_mut_val* value)
{
    if (value == nullptr || yyjson_mut_is_null(value))
    {
        return sol::lua_nil;
    }

    if (yyjson_mut_is_bool(value))
    {
        return sol::make_object(luaState, yyjson_mut_get_bool(value));
    }

    if (yyjson_mut_is_str(value))
    {
        return sol::make_object(luaState,
                                std::string(yyjson_mut_get_str(value), yyjson_mut_get_len(value)));
    }

    if (yyjson_mut_is_int(value))
    {
        return sol::make_object(luaState, yyjson_mut_get_sint(value));
    }

    if (yyjson_mut_is_uint(value))
    {
        return sol::make_object(luaState, yyjson_mut_get_uint(value));
    }

    if (yyjson_mut_is_real(value))
    {
        return sol::make_object(luaState, yyjson_mut_get_real(value));
    }

    if (yyjson_mut_is_arr(value))
    {
        sol::table arrayTable = luaState.create_table();
        std::size_t index = 1;
        yyjson_mut_val* item = nullptr;
        yyjson_mut_arr_iter iterator = {};
        yyjson_mut_arr_iter_init(value, &iterator);
        while ((item = yyjson_mut_arr_iter_next(&iterator)) != nullptr)
        {
            arrayTable[index] = yyjsonMutableValueToLua(luaState, item);
            ++index;
        }
        return arrayTable;
    }

    if (yyjson_mut_is_obj(value))
    {
        sol::table objectTable = luaState.create_table();
        yyjson_mut_val* key = nullptr;
        yyjson_mut_val* item = nullptr;
        yyjson_mut_obj_iter iterator = {};
        yyjson_mut_obj_iter_init(value, &iterator);
        while ((key = yyjson_mut_obj_iter_next(&iterator)) != nullptr)
        {
            item = yyjson_mut_obj_iter_get_val(key);
            objectTable[std::string(yyjson_mut_get_str(key), yyjson_mut_get_len(key))] =
                yyjsonMutableValueToLua(luaState, item);
        }
        return objectTable;
    }

    throw std::runtime_error("beez.data: unsupported JSON value type for deserialization");
}

}  // namespace

bool isLuaArray(const sol::table& table)
{
    if (table.size() == 0)
    {
        return false;
    }

    for (std::size_t index = 1; index <= table.size(); ++index)
    {
        const sol::object Value = table[index];
        if (!Value.valid())
        {
            return false;
        }
    }

    bool onlySequential = true;
    table.for_each([&onlySequential, &table](const sol::object& key, const sol::object& /*value*/)
                    {
                        if (!key.is<int>())
                        {
                            onlySequential = false;
                            return;
                        }

                        const int Key = key.as<int>();
                        if (Key < 1 || static_cast<std::size_t>(Key) > table.size())
                        {
                            onlySequential = false;
                        }
                    });
    return onlySequential;
}

sol::object yyjsonValueToLua(sol::state_view luaState, yyjson_val* value)
{
    return yyjsonImmutableValueToLua(luaState, value);
}

sol::object yyjsonMutValueToLua(sol::state_view luaState, yyjson_mut_val* value)
{
    return yyjsonMutableValueToLua(luaState, value);
}

YyjsonMutDocPtr luaToYyjsonDocument(const sol::object& object)
{
    YyjsonMutDocPtr document(yyjson_mut_doc_new(nullptr));
    if (document == nullptr)
    {
        throw std::runtime_error("beez.data: failed to allocate JSON document");
    }

    yyjson_mut_doc_set_root(document.get(), luaObjectToYyjson(document.get(), object));
    return document;
}

std::string yyjsonDocumentToString(const yyjson_mut_doc& document, const bool pretty)
{
    const yyjson_write_flag Flags =
        pretty ? YYJSON_WRITE_PRETTY : static_cast<yyjson_write_flag>(0);
    char* json = yyjson_mut_write(&document, Flags, nullptr);
    if (json == nullptr)
    {
        throw std::runtime_error("beez.data: failed to serialize JSON");
    }

    const std::string Result(json);
    std::free(json);
    return Result;
}

YyjsonDocPtr parseYyjsonDocument(const std::string& content)
{
    yyjson_doc* document =
        yyjson_read(content.data(), content.size(), YYJSON_READ_NOFLAG);
    if (document == nullptr)
    {
        throw std::runtime_error("beez.data: failed to parse JSON");
    }

    return YyjsonDocPtr(document);
}

}  // namespace beez::plugin::lua::data_detail
