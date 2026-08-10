#include "beez/plugin/lua/api/date/detail/time_point.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace beez::plugin::lua::date_detail
{

namespace
{

[[nodiscard]] std::tm toLocalTime(const std::time_t epochSeconds)
{
    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &epochSeconds);
#else
    localtime_r(&epochSeconds, &localTime);
#endif
    return localTime;
}

[[nodiscard]] std::tm toUtcTime(const std::time_t epochSeconds)
{
    std::tm utcTime {};
#if defined(_WIN32)
    gmtime_s(&utcTime, &epochSeconds);
#else
    gmtime_r(&epochSeconds, &utcTime);
#endif
    return utcTime;
}

[[nodiscard]] std::string formatIso8601FromUtcTm(const std::tm& utcTime, const int millis)
{
    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0')
           << std::setw(3) << millis << 'Z';
    return stream.str();
}

[[nodiscard]] std::string formatIso8601WithOffset(const std::tm& localTime,
                                                  const int millis,
                                                  const int offsetMinutes)
{
    const int AbsoluteMinutes = std::abs(offsetMinutes);
    const int OffsetHours = AbsoluteMinutes / 60;
    const int OffsetRemainderMinutes = AbsoluteMinutes % 60;

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

DateTimeInfo localDateTimeInfo(const std::time_t epochSeconds)
{
    const std::tm LocalTime = toLocalTime(epochSeconds);
    DateTimeInfo info;
    info.year = LocalTime.tm_year + 1900;
    info.month = LocalTime.tm_mon + 1;
    info.day = LocalTime.tm_mday;
    info.hour = LocalTime.tm_hour;
    info.min = LocalTime.tm_min;
    info.sec = LocalTime.tm_sec;
    info.wday = LocalTime.tm_wday + 1;
    info.yday = LocalTime.tm_yday + 1;
    info.isDst = LocalTime.tm_isdst > 0;
    return info;
}

std::string formatLocal(const std::string& pattern, const std::time_t epochSeconds)
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

std::string utcIso8601(const std::time_t epochSeconds)
{
    const auto TimePoint = std::chrono::system_clock::from_time_t(epochSeconds);
    const auto Millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint.time_since_epoch()).count() %
        1000;
    return formatIso8601FromUtcTm(toUtcTime(epochSeconds), static_cast<int>(Millis));
}

std::string utcIso8601WithOffset(const std::time_t epochSeconds, const int offsetMinutes)
{
    const std::time_t ShiftedEpoch = epochSeconds + (static_cast<std::time_t>(offsetMinutes) * 60);
    const auto TimePoint = std::chrono::system_clock::from_time_t(ShiftedEpoch);
    const auto Millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint.time_since_epoch()).count() %
        1000;
    return formatIso8601WithOffset(toUtcTime(ShiftedEpoch),
                                     static_cast<int>(Millis),
                                     offsetMinutes);
}

int localUtcOffsetMinutes()
{
    const std::time_t Now = std::time(nullptr);
    const std::tm LocalTime = toLocalTime(Now);
    const std::tm UtcTime = toUtcTime(Now);

    const int LocalSeconds =
        (((LocalTime.tm_hour * 60) + LocalTime.tm_min) * 60) + LocalTime.tm_sec;
    const int UtcSeconds = (((UtcTime.tm_hour * 60) + UtcTime.tm_min) * 60) + UtcTime.tm_sec;

    int dayDelta = LocalTime.tm_mday - UtcTime.tm_mday;
    if (dayDelta > 1)
    {
        dayDelta = -1;
    }
    else if (dayDelta < -1)
    {
        dayDelta = 1;
    }

    return ((dayDelta * 24 * 60 * 60) + (LocalSeconds - UtcSeconds)) / 60;
}

}  // namespace beez::plugin::lua::date_detail
