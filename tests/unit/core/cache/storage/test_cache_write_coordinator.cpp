#include "beez/core/cache/storage/write_coordinator.hpp"

#include "beez/core/cache/storage/envelope.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"

#include <gtest/gtest.h>

#include <filesystem>

TEST(CacheWriteCoordinatorTest, BuffersUntilFlush)
{
    const auto TempRoot =
        std::filesystem::temp_directory_path() / "beez-cache-write-coordinator-test";
    std::filesystem::remove_all(TempRoot);
    std::filesystem::create_directories(TempRoot);

    beez::core::CacheOptions options;
    options.root = TempRoot;
    options.enabled = true;

    beez::core::CacheWriteCoordinator coordinator(beez::core::CacheWriteStrategy::Phase);
    options.writeCoordinator = &coordinator;

    const auto FilePath = TempRoot / "entry.txt";
    beez::core::writeCacheFile(FilePath, "payload", options);
    EXPECT_FALSE(std::filesystem::exists(FilePath));

    coordinator.flush(options);
    EXPECT_TRUE(std::filesystem::exists(FilePath));
    EXPECT_EQ(beez::core::readCacheFile(FilePath, options), "payload");

    std::filesystem::remove_all(TempRoot);
}
