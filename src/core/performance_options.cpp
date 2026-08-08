#include "beez/core/performance_options.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

}  // namespace

CacheWriteStrategy parseCacheWriteStrategy(const std::string& value)
{
    const std::string Normalized = toLower(value);
    if (Normalized == "immediate")
    {
        return CacheWriteStrategy::Immediate;
    }
    if (Normalized == "phase")
    {
        return CacheWriteStrategy::Phase;
    }
    if (Normalized == "end")
    {
        return CacheWriteStrategy::End;
    }

    throw std::runtime_error("unknown cache write strategy: " + value);
}

const char* toString(CacheWriteStrategy strategy)
{
    switch (strategy)
    {
    case CacheWriteStrategy::Immediate:
        return "immediate";
    case CacheWriteStrategy::Phase:
        return "phase";
    case CacheWriteStrategy::End:
        return "end";
    }
    return "phase";
}

std::vector<const char*> cacheWriteStrategyNames()
{
    return {"immediate", "phase", "end"};
}

PerformanceSettings normalizePerformanceSettings(PerformanceSettings settings)
{
    if (settings.mmapHashingMinBytes == 0)
    {
        settings.mmapHashingMinBytes = 1;
    }
    return settings;
}

}  // namespace beez::core
