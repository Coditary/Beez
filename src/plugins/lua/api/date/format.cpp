#include "beez/plugin/lua/api/date/format.hpp"

#include "beez/plugin/lua/api/date/detail/time_point.hpp"

#include <optional>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindFormat(sol::table& dateTable)
{
    dateTable["format"] =
        [](const std::string& pattern, sol::optional<double> epoch) -> std::string
    {
        std::optional<double> epochValue;
        if (epoch.has_value())
        {
            epochValue = epoch.value();
        }

        return date_detail::formatLocal(pattern,
                                        date_detail::resolveEpochSeconds(epochValue));
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
