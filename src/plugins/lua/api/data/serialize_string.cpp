#include "beez/plugin/lua/api/data/serialize_string.hpp"

#include "beez/plugin/lua/api/data/detail/codec.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindSerializeString(sol::table& dataTable)
{
    dataTable["serialize_string"] = [](const sol::table& table, const sol::object& options) -> std::string
    {
        const data_detail::DataFormat Format = data_detail::resolveFormat(options);
        return data_detail::serializeString(table, Format);
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
