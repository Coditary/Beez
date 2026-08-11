#pragma once

#include <ctime>
#include <optional>
#include <string>

namespace beez::plugin::lua::date_detail
{

struct DateTimeInfo
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    int wday = 0;
    int yday = 0;
    bool isDst = false;
};

[[nodiscard]] std::time_t resolveEpochSeconds(const std::optional<double>& epoch);
[[nodiscard]] double currentEpochSeconds();
[[nodiscard]] DateTimeInfo localDateTimeInfo(std::time_t epochSeconds);
[[nodiscard]] std::string formatLocal(const std::string& pattern, std::time_t epochSeconds);
[[nodiscard]] std::string utcIso8601(std::time_t epochSeconds);
[[nodiscard]] std::string utcIso8601WithOffset(std::time_t epochSeconds, int offsetMinutes);
[[nodiscard]] int localUtcOffsetMinutes();

}  // namespace beez::plugin::lua::date_detail
