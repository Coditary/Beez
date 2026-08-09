#include "beez/core/config/performance_options.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(PerformanceOptionsTest, ParsesCacheWriteStrategies)
{
    EXPECT_EQ(beez::core::parseCacheWriteStrategy("immediate"),
              beez::core::CacheWriteStrategy::Immediate);
    EXPECT_EQ(beez::core::parseCacheWriteStrategy("phase"), beez::core::CacheWriteStrategy::Phase);
    EXPECT_EQ(beez::core::parseCacheWriteStrategy("end"), beez::core::CacheWriteStrategy::End);
}

TEST(PerformanceOptionsTest, RejectsUnknownCacheWriteStrategy)
{
    EXPECT_THROW(static_cast<void>(beez::core::parseCacheWriteStrategy("later")),
                 std::runtime_error);
}

TEST(PerformanceOptionsTest, NormalizesMmapThreshold)
{
    beez::core::PerformanceSettings settings;
    settings.mmapHashingMinBytes = 0;

    const auto Normalized = beez::core::normalizePerformanceSettings(settings);
    EXPECT_EQ(Normalized.mmapHashingMinBytes, 1U);
}
