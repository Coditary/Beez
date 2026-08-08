#pragma once

#include "beez/core/cache_options.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace beez::core
{

enum class CacheWriteStrategy : std::uint8_t
{
    Immediate,
    Phase,
    End,
};

struct PerformanceSettings
{
    CacheWriteStrategy cacheWriteStrategy = CacheWriteStrategy::Phase;
    bool cacheFilesystemMetadata = true;
    bool useMmapForHashing = true;
    std::size_t mmapHashingMinBytes = DefaultMmapHashingMinBytes;
    bool optimizeGcForThroughput = false;
    bool pinThreadsToCores = false;
};

[[nodiscard]] CacheWriteStrategy parseCacheWriteStrategy(const std::string& value);
[[nodiscard]] const char* toString(CacheWriteStrategy strategy);
[[nodiscard]] std::vector<const char*> cacheWriteStrategyNames();

[[nodiscard]] PerformanceSettings normalizePerformanceSettings(PerformanceSettings settings);

}  // namespace beez::core
