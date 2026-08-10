#include "beez/core/cache/fingerprint/content_hash.hpp"
#include "beez/core/config/cache/cache_options.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(ContentHashTest, HashBytesIsDeterministic)
{
    const auto Hasher = beez::core::makeSha256Hasher();
    EXPECT_EQ(Hasher->hashBytes("hello"), Hasher->hashBytes("hello"));
    EXPECT_NE(Hasher->hashBytes("hello"), Hasher->hashBytes("world"));
}

TEST(ContentHashTest, CombineProducesStableDigest)
{
    const auto Hasher = beez::core::makeSha256Hasher();
    const std::string Combined = Hasher->combine({"alpha", "beta", "gamma"});
    EXPECT_EQ(Combined.size(), 16U);
    EXPECT_EQ(Combined, Hasher->combine({"alpha", "beta", "gamma"}));
    EXPECT_NE(Combined, Hasher->combine({"alpha", "gamma", "beta"}));
}

TEST(ContentHashTest, HashFileReflectsContent)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_hash_file_test";
    const auto FilePath = Directory / "input.txt";
    writeFile(FilePath, "cache-me");

    const auto Hasher = beez::core::makeSha256Hasher();
    const std::string First = Hasher->hashFile(FilePath);
    writeFile(FilePath, "cache-me-updated");
    const std::string Second = Hasher->hashFile(FilePath);

    std::filesystem::remove_all(Directory);

    EXPECT_EQ(First.size(), 16U);
    EXPECT_NE(First, Second);
}

TEST(ContentHashTest, AlgorithmsProduceDifferentDigests)
{
    beez::core::ContentHashSettings base;
    base.algorithm = beez::core::ContentHashAlgorithm::Fnv1a64;
    const auto FnvHasher = beez::core::makeContentHasher(base);

    base.algorithm = beez::core::ContentHashAlgorithm::Crc32;
    const auto CrcHasher = beez::core::makeContentHasher(base);

    const std::string Digest = FnvHasher->hashBytes("beez");
    EXPECT_NE(Digest, CrcHasher->hashBytes("beez"));
}

TEST(ContentHashTest, SeedChangesDigest)
{
    beez::core::ContentHashSettings settings;
    settings.algorithm = beez::core::ContentHashAlgorithm::Fnv1a64;
    settings.seed = 0U;
    const auto DefaultHasher = beez::core::makeContentHasher(settings);

    settings.seed = 7U;
    const auto SeededHasher = beez::core::makeContentHasher(settings);

    EXPECT_NE(DefaultHasher->hashBytes("beez"), SeededHasher->hashBytes("beez"));
}

TEST(ContentHashTest, AdditionalAlgorithmsProduceDigests)
{
    beez::core::ContentHashSettings settings;
    settings.algorithm = beez::core::ContentHashAlgorithm::Fnv1a32;
    const auto Fnv32Hasher = beez::core::makeContentHasher(settings);

    settings.algorithm = beez::core::ContentHashAlgorithm::Djb2;
    const auto Djb2Hasher = beez::core::makeContentHasher(settings);

    settings.algorithm = beez::core::ContentHashAlgorithm::Sdbm;
    const auto SdbmHasher = beez::core::makeContentHasher(settings);

    const std::string Payload = "hash-payload";
    EXPECT_EQ(Fnv32Hasher->hashBytes(Payload).size(), 8U);
    EXPECT_EQ(Djb2Hasher->hashBytes(Payload).size(), 8U);
    EXPECT_EQ(SdbmHasher->hashBytes(Payload).size(), 8U);
    EXPECT_NE(Fnv32Hasher->hashBytes(Payload), Djb2Hasher->hashBytes(Payload));
}

TEST(ContentHashTest, HashFileUsesMmapForLargeFilesOnLinux)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_hash_mmap_test";
    const auto FilePath = Directory / "large.bin";
    std::filesystem::remove_all(Directory);

    const std::string Payload(70000, 'x');
    writeFile(FilePath, Payload);

    beez::core::ContentHashSettings settings;
    settings.algorithm = beez::core::ContentHashAlgorithm::Crc32;
    settings.useMmapForHashing = true;
    settings.mmapHashingMinBytes = 1U;
    const auto MmapHasher = beez::core::makeContentHasher(settings);

    settings.useMmapForHashing = false;
    const auto StreamHasher = beez::core::makeContentHasher(settings);

    EXPECT_EQ(MmapHasher->hashFile(FilePath), StreamHasher->hashFile(FilePath));

    std::filesystem::remove_all(Directory);
}

TEST(ContentHashTest, HashFileReturnsDigestForMissingFile)
{
    const auto Hasher = beez::core::makeSha256Hasher();
    const std::string Digest =
        Hasher->hashFile(std::filesystem::temp_directory_path() / "beez_missing_hash_file.bin");
    EXPECT_EQ(Digest.size(), 16U);
}
