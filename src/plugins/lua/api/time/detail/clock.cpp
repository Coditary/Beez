#include "beez/plugin/lua/api/time/detail/clock.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace beez::plugin::lua::time_detail
{

namespace
{

constexpr std::int64_t MillisPerSecond = 1000;
constexpr double MillisPerSecondDouble = 1000.0;

[[nodiscard]] std::int64_t currentEpochMillis()
{
    const auto Now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Now.time_since_epoch()).count();
}

[[nodiscard]] std::string formatIso8601Utc(const std::chrono::system_clock::time_point& timePoint)
{
    const auto TimeT = std::chrono::system_clock::to_time_t(timePoint);
    const auto Millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch())
            .count() %
        MillisPerSecond;

    std::tm utcTime {};
#ifdef _WIN32
    gmtime_s(&utcTime, &TimeT);
#else
    gmtime_r(&TimeT, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0')
           << std::setw(3) << Millis << 'Z';
    return stream.str();
}

}  // namespace

std::string nowMillisString()
{
    return std::to_string(currentEpochMillis());
}

std::string uptimeMillisString()
{
#ifdef __linux__
    // NOLINTBEGIN(misc-include-cleaner)
    struct sysinfo info {};
    if (sysinfo(&info) == 0)
    {
        return std::to_string(static_cast<std::int64_t>(info.uptime) * MillisPerSecond);
    }

    std::ifstream stream("/proc/uptime");
    if (stream.is_open())
    {
        double uptimeSeconds = 0.0;
        stream >> uptimeSeconds;
        if (uptimeSeconds > 0.0)
        {
            return std::to_string(static_cast<std::int64_t>(uptimeSeconds * MillisPerSecondDouble));
        }
    }
// NOLINTEND(misc-include-cleaner)
#endif

    return "0";
}

std::string iso8601UtcNow()
{
    return formatIso8601Utc(std::chrono::system_clock::now());
}

void sleepMillis(std::int64_t milliseconds)
{
    if (milliseconds <= 0)
    {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void sleepSeconds(double seconds)
{
    if (seconds <= 0.0)
    {
        return;
    }

    const auto Duration = std::chrono::duration<double>(seconds);
    std::this_thread::sleep_for(Duration);
}

}  // namespace beez::plugin::lua::time_detail
