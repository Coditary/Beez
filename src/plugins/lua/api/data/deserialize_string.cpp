#include "beez/plugin/lua/api/data/deserialize_string.hpp"

#include "beez/plugin/lua/api/data/detail/codec.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindDeserializeString(sol::table& dataTable, const std::shared_ptr<sol::state>& luaState)
{
    dataTable["deserialize_string"] = [luaState](const std::string& content,
                                                 const sol::object& options) -> sol::table
    {
        const data_detail::DataFormat Format = data_detail::resolveFormat(options);
        return data_detail::deserializeString(*luaState, content, Format);
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
