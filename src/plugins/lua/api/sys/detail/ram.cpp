#include "beez/plugin/lua/api/sys/detail/ram.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace beez::plugin::lua::sys_detail
{

namespace
{

constexpr std::uint64_t BytesPerKilobyte = 1024U;

[[nodiscard]] bool parseMemInfoValueKb(const std::string& key, std::uint64_t& value)
{
    std::ifstream stream("/proc/meminfo");
    if (!stream.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.starts_with(key))
        {
            continue;
        }

        std::istringstream parser(line.substr(key.size()));
        std::uint64_t kilobytes = 0;
        parser >> kilobytes;
        value = kilobytes * BytesPerKilobyte;
        return true;
    }

    return false;
}

#ifdef __linux__
// NOLINTBEGIN(misc-include-cleaner)
[[nodiscard]] std::uint64_t ramTotalFromSysInfo()
{
    struct sysinfo info {};
    if (sysinfo(&info) != 0)
    {
        return 0;
    }

    return static_cast<std::uint64_t>(info.totalram) * info.mem_unit;
}

[[nodiscard]] std::uint64_t ramFreeFromSysInfo()
{
    struct sysinfo info {};
    if (sysinfo(&info) != 0)
    {
        return 0;
    }

    return static_cast<std::uint64_t>(info.freeram) * info.mem_unit;
}
// NOLINTEND(misc-include-cleaner)
#endif

}  // namespace

std::uint64_t ramTotalBytes()
{
    std::uint64_t total = 0;
    if (parseMemInfoValueKb("MemTotal:", total))
    {
        return total;
    }

#ifdef __linux__
    if (const std::uint64_t FromSysInfo = ramTotalFromSysInfo(); FromSysInfo > 0)
    {
        return FromSysInfo;
    }
#endif

    throw std::runtime_error("beez.sys.ram_total: unable to determine system memory");
}

std::uint64_t ramFreeBytes()
{
    std::uint64_t available = 0;
    if (parseMemInfoValueKb("MemAvailable:", available))
    {
        return available;
    }

    if (parseMemInfoValueKb("MemFree:", available))
    {
        return available;
    }

#ifdef __linux__
    if (const std::uint64_t FromSysInfo = ramFreeFromSysInfo(); FromSysInfo > 0)
    {
        return FromSysInfo;
    }
#endif

    throw std::runtime_error("beez.sys.ram_free: unable to determine available memory");
}

}  // namespace beez::plugin::lua::sys_detail
