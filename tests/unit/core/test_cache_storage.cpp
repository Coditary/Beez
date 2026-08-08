#include "beez/core/cache_storage.hpp"

#include "beez/core/cache_options.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
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

[[nodiscard]] std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
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

TEST(CacheStorageTest, ReadUsesEnvelopeAlgorithmNotCurrentConfig)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_cache_header_read_test";
    const auto FilePath = Directory / "entry.manifest";

    beez::core::CacheOptions writeOptions;
    writeOptions.compress.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    writeOptions.compress.level = 6;

    const std::string Payload = "step=lint\noutput=report/lint.ok\n";
    beez::core::writeCacheFile(FilePath, Payload, writeOptions);

    beez::core::CacheOptions readOptions;
    readOptions.compress.algorithm = beez::core::CacheCompressionAlgorithm::Zlib;
    readOptions.compress.level = 9;
    EXPECT_EQ(beez::core::readCacheFile(FilePath, readOptions), Payload);

    std::filesystem::remove_all(Directory);
}

TEST(CacheStorageTest, MigratesCompressedFilesWhenCompressionConfigChanges)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_cache_migrate_test";
    const auto FilePath = Directory / "entries" / "entry.manifest";

    beez::core::CacheOptions options;
    options.root = Directory;
    options.compress.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    options.compress.level = 6;

    const std::string Payload = "step=format\noutput=src/main.cpp\n";
    beez::core::writeCacheFile(FilePath, Payload, options);
    beez::core::updateCacheStorage(options);

    options.compress.algorithm = beez::core::CacheCompressionAlgorithm::Zlib;
    options.compress.level = 9;
    const std::size_t MigratedFiles = beez::core::updateCacheStorage(options);
    EXPECT_EQ(MigratedFiles, 1U);

    const std::string OnDisk = readFile(FilePath);
    EXPECT_NE(OnDisk.find("algorithm=zlib"), std::string::npos);
    EXPECT_NE(OnDisk.find("level=9"), std::string::npos);
    EXPECT_EQ(beez::core::readCacheFile(FilePath, options), Payload);

    std::filesystem::remove_all(Directory);
}

TEST(CacheStorageTest, SkipsMigrationWhenCompressionConfigUnchanged)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_cache_skip_migrate_test";
    const auto FilePath = Directory / "entry.manifest";

    beez::core::CacheOptions options;
    options.root = Directory;
    options.compress.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    options.compress.level = 6;

    beez::core::writeCacheFile(FilePath, "step=lint\n", options);
    beez::core::updateCacheStorage(options);
    const auto Before = std::filesystem::last_write_time(FilePath);

    const std::size_t MigratedFiles = beez::core::updateCacheStorage(options);
    EXPECT_EQ(MigratedFiles, 0U);
    EXPECT_EQ(std::filesystem::last_write_time(FilePath), Before);

    std::filesystem::remove_all(Directory);
}

TEST(CacheStorageTest, ReadFailsForMissingFile)
{
    const beez::core::CacheOptions Options;
    EXPECT_THROW(
        static_cast<void>(beez::core::readCacheFile("/tmp/does-not-exist-beez-cache", Options)),
        std::runtime_error);
}
