#pragma once

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

#include <yyjson.h>

#include <memory>
#include <string>

namespace beez::plugin::lua::data_detail
{

[[nodiscard]] bool isLuaArray(const sol::table& table);

struct YyjsonDocDeleter
{
    void operator()(yyjson_doc* document) const
    {
        if (document != nullptr)
        {
            yyjson_doc_free(document);
        }
    }
};

struct YyjsonMutDocDeleter
{
    void operator()(yyjson_mut_doc* document) const
    {
        if (document != nullptr)
        {
            yyjson_mut_doc_free(document);
        }
    }
};

using YyjsonDocPtr = std::unique_ptr<yyjson_doc, YyjsonDocDeleter>;
using YyjsonMutDocPtr = std::unique_ptr<yyjson_mut_doc, YyjsonMutDocDeleter>;

[[nodiscard]] sol::object yyjsonValueToLua(sol::state_view luaState, yyjson_val* value);
[[nodiscard]] sol::object yyjsonMutValueToLua(sol::state_view luaState, yyjson_mut_val* value);
[[nodiscard]] YyjsonMutDocPtr luaToYyjsonDocument(const sol::object& object);
[[nodiscard]] std::string yyjsonDocumentToString(const yyjson_mut_doc& document, bool pretty);
[[nodiscard]] YyjsonDocPtr parseYyjsonDocument(const std::string& content);

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
