#include "beez/core/cache_storage.hpp"

#include "beez/core/cache_options.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream << content;
}

[[nodiscard]] bool isReadOnlyForOwner(const std::filesystem::path& path)
{
    const auto Permissions = std::filesystem::status(path).permissions();
    const auto OwnerWrite = std::filesystem::perms::owner_write;
    return (Permissions & OwnerWrite) == std::filesystem::perms::none;
}

}  // namespace

TEST(CacheStorageTest, ProtectMakesWrittenFilesReadOnly)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_cache_protect_test";
    const auto FilePath = Directory / "entry.manifest";

    beez::core::CacheOptions options;
    options.protect = true;
    options.compress.algorithm = beez::core::CacheCompressionAlgorithm::None;

    beez::core::writeCacheFile(FilePath, "step=compile\n", options);
    ASSERT_TRUE(std::filesystem::exists(FilePath));
    EXPECT_TRUE(isReadOnlyForOwner(FilePath));

    beez::core::prepareCacheFileForWrite(FilePath, true);
    EXPECT_FALSE(isReadOnlyForOwner(FilePath));

    std::filesystem::remove_all(Directory);
}

TEST(CacheStorageTest, WriteAndReadRoundTripWithCompression)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_cache_roundtrip_test";
    const auto FilePath = Directory / "entry.manifest";

    beez::core::CacheOptions options;
    options.compress.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    options.compress.level = 6;

    const std::string Payload = "step=lint\noutput=report/lint.ok\n";
    beez::core::writeCacheFile(FilePath, Payload, options);
    EXPECT_EQ(beez::core::readCacheFile(FilePath, options), Payload);

    std::filesystem::remove_all(Directory);
}

TEST(CacheStorageTest, ReadFailsForMissingFile)
{
    const beez::core::CacheOptions Options;
    EXPECT_THROW(
        static_cast<void>(beez::core::readCacheFile("/tmp/does-not-exist-beez-cache", Options)),
        std::runtime_error);
}
