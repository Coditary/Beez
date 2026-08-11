#include "beez/plugin/lua/api/date/detail/time_point.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::date_detail
{

namespace
{

constexpr int EpochYearOffset = 1900;
constexpr int MonOneBased = 1;
constexpr int HoursPerDay = 24;
constexpr int MinutesPerHour = 60;
constexpr int SecondsPerMinute = 60;
constexpr int MillisPerSecond = 1000;

[[nodiscard]] std::tm toLocalTime(std::time_t epochSeconds)
{
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &epochSeconds);
#else
    localtime_r(&epochSeconds, &localTime);
#endif
    return localTime;
}

[[nodiscard]] std::tm toUtcTime(std::time_t epochSeconds)
{
    std::tm utcTime {};
#ifdef _WIN32
    gmtime_s(&utcTime, &epochSeconds);
#else
    gmtime_r(&epochSeconds, &utcTime);
#endif
    return utcTime;
}

[[nodiscard]] std::string formatIso8601FromUtcTm(const std::tm& utcTime, int millis)
{
    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0')
           << std::setw(3) << millis << 'Z';
    return stream.str();
}

[[nodiscard]] std::string
formatIso8601WithOffset(const std::tm& localTime, int millis, int offsetMinutes)
{
    const int AbsoluteMinutes = std::abs(offsetMinutes);
    const int OffsetHours = AbsoluteMinutes / MinutesPerHour;
    const int OffsetRemainderMinutes = AbsoluteMinutes % MinutesPerHour;

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0')
           << std::setw(3) << millis << (offsetMinutes >= 0 ? '+' : '-') << std::setfill('0')
           << std::setw(2) << OffsetHours << ':' << std::setfill('0') << std::setw(2)
           << OffsetRemainderMinutes;
    return stream.str();
}

}  // namespace

std::time_t resolveEpochSeconds(const std::optional<double>& epoch)
{
    if (!epoch.has_value())
    {
        return std::time(nullptr);
    }

    return static_cast<std::time_t>(epoch.value());
}

double currentEpochSeconds()
{
    const auto Now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(Now.time_since_epoch())
        .count();
}

DateTimeInfo localDateTimeInfo(std::time_t epochSeconds)
{
    const std::tm LocalTime = toLocalTime(epochSeconds);
    DateTimeInfo info;
    info.year = LocalTime.tm_year + EpochYearOffset;
    info.month = LocalTime.tm_mon + MonOneBased;
    info.day = LocalTime.tm_mday;
    info.hour = LocalTime.tm_hour;
    info.min = LocalTime.tm_min;
    info.sec = LocalTime.tm_sec;
    info.wday = LocalTime.tm_wday + MonOneBased;
    info.yday = LocalTime.tm_yday + MonOneBased;
    info.isDst = LocalTime.tm_isdst > 0;
    return info;
}

std::string formatLocal(const std::string& pattern, std::time_t epochSeconds)
{
    const std::tm LocalTime = toLocalTime(epochSeconds);
    std::ostringstream stream;
    stream << std::put_time(&LocalTime, pattern.c_str());
    if (stream.fail())
    {
        throw std::runtime_error("beez.date.format: invalid format string");
    }

    return stream.str();
}

std::string utcIso8601(std::time_t epochSeconds)
{
    const auto TimePoint = std::chrono::system_clock::from_time_t(epochSeconds);
    const auto Millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint.time_since_epoch())
            .count() %
        MillisPerSecond;
    return formatIso8601FromUtcTm(toUtcTime(epochSeconds), static_cast<int>(Millis));
}

std::string utcIso8601WithOffset(std::time_t epochSeconds, int offsetMinutes)
{
    const std::time_t ShiftedEpoch =
        epochSeconds + (static_cast<std::time_t>(offsetMinutes) * SecondsPerMinute);
    const auto TimePoint = std::chrono::system_clock::from_time_t(ShiftedEpoch);
    const auto Millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint.time_since_epoch())
            .count() %
        MillisPerSecond;
    return formatIso8601WithOffset(
        toUtcTime(ShiftedEpoch), static_cast<int>(Millis), offsetMinutes);
}

int localUtcOffsetMinutes()
{
    const std::time_t Now = std::time(nullptr);
    const std::tm LocalTime = toLocalTime(Now);
    const std::tm UtcTime = toUtcTime(Now);

    const int LocalSeconds =
        (((LocalTime.tm_hour * MinutesPerHour) + LocalTime.tm_min) * SecondsPerMinute) +
        LocalTime.tm_sec;
    const int UtcSeconds =
        (((UtcTime.tm_hour * MinutesPerHour) + UtcTime.tm_min) * SecondsPerMinute) + UtcTime.tm_sec;

    int dayDelta = LocalTime.tm_mday - UtcTime.tm_mday;
    if (dayDelta > 1)
    {
        dayDelta = -1;
    }
    else if (dayDelta < -1)
    {
        dayDelta = 1;
    }

    return ((dayDelta * HoursPerDay * MinutesPerHour * SecondsPerMinute) +
            (LocalSeconds - UtcSeconds)) /
           MinutesPerHour;
}

}  // namespace beez::plugin::lua::date_detail
