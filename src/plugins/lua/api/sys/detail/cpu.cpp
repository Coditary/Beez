#include "beez/plugin/lua/api/sys/detail/cpu.hpp"

#include <cstddef>
#include <set>
#include <string>
#include <thread>

#ifdef __linux__
#include <fstream>
#include <unistd.h>
#endif

namespace beez::plugin::lua::sys_detail
{

namespace
{

[[nodiscard]] std::size_t fallbackThreadCount()
{
    const auto Count = std::thread::hardware_concurrency();
    return Count == 0 ? 1U : Count;
}

#ifdef __linux__

[[nodiscard]] std::size_t cpuCoresFromProcCpuInfo()
{
    std::ifstream stream("/proc/cpuinfo");
    if (!stream.is_open())
    {
        return 0;
    }

    std::set<std::string> cores;
    std::string line;
    std::string physicalId;
    std::string coreId;

    const auto FlushCore = [&cores, &physicalId, &coreId]()
    {
        if (!coreId.empty())
        {
            cores.insert(physicalId + ":" + coreId);
        }

        physicalId.clear();
        coreId.clear();
    };

    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            FlushCore();
            continue;
        }

        if (line.starts_with("physical id"))
        {
            FlushCore();
            const auto Delimiter = line.find(':');
            if (Delimiter != std::string::npos)
            {
                physicalId = line.substr(Delimiter + 1);
                const auto First = physicalId.find_first_not_of(" \t");
                if (First != std::string::npos)
                {
                    physicalId.erase(0, First);
                }
            }

            continue;
        }

        if (line.starts_with("core id"))
        {
            const auto Delimiter = line.find(':');
            if (Delimiter != std::string::npos)
            {
                coreId = line.substr(Delimiter + 1);
                const auto First = coreId.find_first_not_of(" \t");
                if (First != std::string::npos)
                {
                    coreId.erase(0, First);
                }
            }
        }
    }

    FlushCore();
    return cores.empty() ? 0U : cores.size();
}

#endif

}  // namespace

std::size_t cpuThreadCount()
{
    return fallbackThreadCount();
}

std::size_t cpuCoreCount()
{
#ifdef __linux__
    if (const std::size_t Cores = cpuCoresFromProcCpuInfo(); Cores > 0)
    {
        return Cores;
    }

    const long Online = sysconf(_SC_NPROCESSORS_ONLN);
    if (Online > 0)
    {
        return static_cast<std::size_t>(Online);
    }
#endif

    return fallbackThreadCount();
}

}  // namespace beez::plugin::lua::sys_detail
