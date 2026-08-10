#include "beez/plugin/lua/api/data/detail/schema.hpp"

#include <format>
#include <string>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] bool validateAt(yyjson_val* data,
                              yyjson_val* schema,
                              const std::string& path,
                              std::string& error);

[[nodiscard]] bool typeMatches(yyjson_val* data, const std::string& type)
{
    if (type == "null")
    {
        return yyjson_is_null(data);
    }

    if (type == "boolean")
    {
        return yyjson_is_bool(data);
    }

    if (type == "string")
    {
        return yyjson_is_str(data);
    }

    if (type == "integer")
    {
        return yyjson_is_int(data);
    }

    if (type == "number")
    {
        return yyjson_is_num(data);
    }

    if (type == "array")
    {
        return yyjson_is_arr(data);
    }

    if (type == "object")
    {
        return yyjson_is_obj(data);
    }

    return false;
}

[[nodiscard]] bool validateAt(yyjson_val* data,
                              yyjson_val* schema,
                              const std::string& path,
                              std::string& error)
{
    yyjson_val* typeValue = yyjson_obj_get(schema, "type");
    if (typeValue != nullptr && yyjson_is_str(typeValue))
    {
        const std::string Type(yyjson_get_str(typeValue), yyjson_get_len(typeValue));
        if (!typeMatches(data, Type))
        {
            error = std::format("beez.data.validate: '{}' expected type '{}'", path, Type);
            return false;
        }
    }

    yyjson_val* enumValue = yyjson_obj_get(schema, "enum");
    if (enumValue != nullptr && yyjson_is_arr(enumValue))
    {
        bool matched = false;
        yyjson_val* option = nullptr;
        yyjson_arr_iter iterator = {};
        yyjson_arr_iter_init(enumValue, &iterator);
        while ((option = yyjson_arr_iter_next(&iterator)) != nullptr)
        {
            if (yyjson_equals(data, option))
            {
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            error = std::format("beez.data.validate: '{}' value not in enum", path);
            return false;
        }
    }

    if (yyjson_is_obj(data))
    {
        yyjson_val* requiredValue = yyjson_obj_get(schema, "required");
        if (requiredValue != nullptr && yyjson_is_arr(requiredValue))
        {
            yyjson_val* requiredField = nullptr;
            yyjson_arr_iter requiredIterator = {};
            yyjson_arr_iter_init(requiredValue, &requiredIterator);
            while ((requiredField = yyjson_arr_iter_next(&requiredIterator)) != nullptr)
            {
                if (!yyjson_is_str(requiredField))
                {
                    continue;
                }

                const std::string Field(yyjson_get_str(requiredField),
                                        yyjson_get_len(requiredField));
                if (yyjson_obj_get(data, Field.c_str()) == nullptr)
                {
                    const std::string FieldPath = path.empty() ? Field : path + '.' + Field;
                    error = std::format("beez.data.validate: missing required field '{}'",
                                        FieldPath);
                    return false;
                }
            }
        }

        yyjson_val* propertiesValue = yyjson_obj_get(schema, "properties");
        if (propertiesValue != nullptr && yyjson_is_obj(propertiesValue))
        {
            yyjson_val* fieldKey = nullptr;
            yyjson_val* fieldSchema = nullptr;
            yyjson_obj_iter propertiesIterator = {};
            yyjson_obj_iter_init(propertiesValue, &propertiesIterator);
            while ((fieldKey = yyjson_obj_iter_next(&propertiesIterator)) != nullptr)
            {
                fieldSchema = yyjson_obj_iter_get_val(fieldKey);
                const std::string Field(yyjson_get_str(fieldKey), yyjson_get_len(fieldKey));
                yyjson_val* fieldData = yyjson_obj_get(data, Field.c_str());
                if (fieldData == nullptr)
                {
                    continue;
                }

                const std::string FieldPath = path.empty() ? Field : path + '.' + Field;
                if (!validateAt(fieldData, fieldSchema, FieldPath, error))
                {
                    return false;
                }
            }
        }

        yyjson_val* additionalProperties = yyjson_obj_get(schema, "additionalProperties");
        if (additionalProperties != nullptr && yyjson_is_bool(additionalProperties) &&
            !yyjson_get_bool(additionalProperties) && propertiesValue != nullptr &&
            yyjson_is_obj(propertiesValue))
        {
            yyjson_val* fieldKey = nullptr;
            yyjson_obj_iter dataIterator = {};
            yyjson_obj_iter_init(data, &dataIterator);
            while ((fieldKey = yyjson_obj_iter_next(&dataIterator)) != nullptr)
            {
                const std::string Field(yyjson_get_str(fieldKey), yyjson_get_len(fieldKey));
                if (yyjson_obj_get(propertiesValue, Field.c_str()) == nullptr)
                {
                    const std::string FieldPath = path.empty() ? Field : path + '.' + Field;
                    error = std::format("beez.data.validate: additional property '{}' not allowed",
                                        FieldPath);
                    return false;
                }
            }
        }
    }

    if (yyjson_is_arr(data))
    {
        yyjson_val* itemsSchema = yyjson_obj_get(schema, "items");
        if (itemsSchema != nullptr)
        {
            std::size_t index = 0;
            yyjson_val* item = nullptr;
            yyjson_arr_iter iterator = {};
            yyjson_arr_iter_init(data, &iterator);
            while ((item = yyjson_arr_iter_next(&iterator)) != nullptr)
            {
                const std::string ItemPath = std::format("{}[{}]", path, index);
                if (!validateAt(item, itemsSchema, ItemPath, error))
                {
                    return false;
                }
                ++index;
            }
        }
    }

    return true;
}

}  // namespace

bool validateJson(yyjson_val* data, yyjson_val* schema, std::string& error)
{
    return validateAt(data, schema, "", error);
}

}  // namespace beez::plugin::lua::data_detail
