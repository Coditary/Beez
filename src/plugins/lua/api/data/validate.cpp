#include "beez/plugin/lua/api/data/validate.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"
#include "beez/plugin/lua/api/data/schema.hpp"

#include <stdexcept>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindValidate(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["validate"] = [luaState](const sol::table& data,
                                       const sol::object& schemaOrString) -> bool
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
            const std::string SchemaJson =
                data_detail::yyjsonDocumentToString(*SchemaMutable, false);
            SchemaDocument = data_detail::parseYyjsonDocument(SchemaJson);
        }
        else
        {
            throw std::runtime_error("beez.data.validate: schema must be a JSON string or table");
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
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
