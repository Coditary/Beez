#include "beez/plugin/lua/api/data/deserialize_file.hpp"

#include "beez/plugin/lua/api/data/detail/codec.hpp"
#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"
#include "beez/plugin/lua/api/data/detail/schema.hpp"
#include "beez/plugin/lua/api/data/detail/table_ops.hpp"
#include "beez/plugin/lua/api/fs/detail/operations.hpp"

#include <filesystem>
#include <stdexcept>

namespace beez::plugin::lua
{

void bindDeserializeFile(sol::table& dataTable,
                         const std::shared_ptr<sol::state>& luaState,
                         const core::Context& context)
{
    dataTable["deserialize_file"] =
        [luaState, &context](const std::string& path, const sol::object& options) -> sol::table
    {
        const std::filesystem::path Resolved = fs_detail::resolvedPath(context, path);
        const data_detail::DataFormat Format = data_detail::resolveFormat(Resolved, options);
        return data_detail::deserializeFile(*luaState, Resolved, Format);
    };
}

void bindSerializeFile(sol::table& dataTable, const core::Context& context)
{
    dataTable["serialize_file"] =
        [&context](const std::string& path, const sol::table& table, const sol::object& options)
    {
        const std::filesystem::path Resolved = fs_detail::resolvedPath(context, path);
        const data_detail::DataFormat Format = data_detail::resolveFormat(Resolved, options);
        data_detail::serializeFile(Resolved, table, Format);
    };
}

void bindDeserializeString(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["deserialize_string"] =
        [luaState](const std::string& content, const sol::object& options) -> sol::table
    {
        const data_detail::DataFormat Format = data_detail::resolveFormat(options);
        return data_detail::deserializeString(*luaState, content, Format);
    };
}

void bindSerializeString(sol::table& dataTable)
{
    dataTable["serialize_string"] = [](const sol::table& table, const sol::object& options) -> std::string
    {
        const data_detail::DataFormat Format = data_detail::resolveFormat(options);
        return data_detail::serializeString(table, Format);
    };
}

void bindMerge(sol::table& dataTable)
{
    dataTable["merge"] = [](sol::table target, const sol::table& source) -> sol::table
    {
        data_detail::deepMerge(target, source);
        return target;
    };
}

void bindClone(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["clone"] = [](const sol::table& table) -> sol::table
    { return data_detail::cloneTable(sol::state_view(table.lua_state()), table); };
}

void bindGet(sol::table& dataTable)
{
    dataTable["get"] =
        [](const sol::table& table, const std::string& path, const sol::object& defaultValue) -> sol::object
    { return data_detail::getPath(table, path, defaultValue); };
}

void bindSet(sol::table& dataTable)
{
    dataTable["set"] = [](sol::table table, const std::string& path, const sol::object& value) -> sol::table
    {
        data_detail::setPath(table, path, value);
        return table;
    };
}

void bindDiff(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["diff"] = [](const sol::table& left, const sol::table& right) -> sol::table
    { return data_detail::diffTables(sol::state_view(left.lua_state()), left, right); };
}

void bindValidate(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["validate"] =
        [luaState](const sol::table& data, const sol::object& schemaOrString) -> bool
    {
        const data_detail::YyjsonMutDocPtr DataDocument =
            data_detail::luaToYyjsonDocument(sol::make_object(*luaState, data));
        const std::string DataJson = data_detail::yyjsonDocumentToString(*DataDocument, false);
        const data_detail::YyjsonDocPtr ImmutableData = data_detail::parseYyjsonDocument(DataJson);

        data_detail::YyjsonDocPtr SchemaDocument;
        if (schemaOrString.is<std::string>())
        {
            SchemaDocument = data_detail::parseYyjsonDocument(schemaOrString.as<std::string>());
        }
        else if (schemaOrString.is<sol::table>())
        {
            const data_detail::YyjsonMutDocPtr SchemaMutable =
                data_detail::luaToYyjsonDocument(sol::make_object(*luaState, schemaOrString));
            const std::string SchemaJson = data_detail::yyjsonDocumentToString(*SchemaMutable, false);
            SchemaDocument = data_detail::parseYyjsonDocument(SchemaJson);
        }
        else
        {
            throw std::runtime_error(
                "beez.data.validate: schema must be a JSON string or table");
        }

        std::string error;
        if (!data_detail::validateJson(yyjson_doc_get_root(ImmutableData.get()),
                                       yyjson_doc_get_root(SchemaDocument.get()),
                                       error))
        {
            throw std::runtime_error(error);
        }

        return true;
    };
}

}  // namespace beez::plugin::lua
