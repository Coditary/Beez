#include "beez/plugin/lua/api/date/info.hpp"

#include "beez/plugin/lua/api/date/detail/time_point.hpp"

#include <optional>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

void bindInfo(sol::table& dateTable, const std::shared_ptr<sol::state>& luaState)
{
    dateTable["info"] = [luaState](sol::optional<double> epoch) -> sol::table
    {
        std::optional<double> epochValue;
        if (epoch.has_value())
        {
            epochValue = epoch.value();
        }

        const date_detail::DateTimeInfo Info =
            date_detail::localDateTimeInfo(date_detail::resolveEpochSeconds(epochValue));

        sol::table result = luaState->create_table();
        result["year"] = Info.year;
        result["month"] = Info.month;
        result["day"] = Info.day;
        result["hour"] = Info.hour;
        result["min"] = Info.min;
        result["sec"] = Info.sec;
        result["wday"] = Info.wday;
        result["yday"] = Info.yday;
        result["is_dst"] = Info.isDst;
        return result;
    };
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
