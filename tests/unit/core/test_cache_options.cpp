#include "beez/core/cache_options.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

TEST(CacheOptionsTest, ParsesHashAlgorithms)
{
    EXPECT_EQ(beez::core::parseContentHashAlgorithm("fnv1a64"),
              beez::core::ContentHashAlgorithm::Fnv1a64);
    EXPECT_EQ(beez::core::parseContentHashAlgorithm("fnv1a32"),
              beez::core::ContentHashAlgorithm::Fnv1a32);
    EXPECT_EQ(beez::core::parseContentHashAlgorithm("crc32"),
              beez::core::ContentHashAlgorithm::Crc32);
    EXPECT_EQ(beez::core::parseContentHashAlgorithm("djb2"),
              beez::core::ContentHashAlgorithm::Djb2);
    EXPECT_EQ(beez::core::parseContentHashAlgorithm("sdbm"),
              beez::core::ContentHashAlgorithm::Sdbm);
}

TEST(CacheOptionsTest, RejectsUnknownHashAlgorithm)
{
    EXPECT_THROW(static_cast<void>(beez::core::parseContentHashAlgorithm("sha512")),
                 std::runtime_error);
}

TEST(CacheOptionsTest, ParsesCompressionAlgorithms)
{
    EXPECT_EQ(beez::core::parseCacheCompressionAlgorithm("none"),
              beez::core::CacheCompressionAlgorithm::None);
    EXPECT_EQ(beez::core::parseCacheCompressionAlgorithm("gzip"),
              beez::core::CacheCompressionAlgorithm::Gzip);
    EXPECT_EQ(beez::core::parseCacheCompressionAlgorithm("zlib"),
              beez::core::CacheCompressionAlgorithm::Zlib);
    EXPECT_EQ(beez::core::parseCacheCompressionAlgorithm("rle"),
              beez::core::CacheCompressionAlgorithm::Rle);
    EXPECT_EQ(beez::core::parseCacheCompressionAlgorithm("deflate"),
              beez::core::CacheCompressionAlgorithm::Deflate);
}

TEST(CacheOptionsTest, NormalizesCompressionLevel)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    settings.level = 99;

    const auto Normalized = beez::core::normalizeCacheCompressionSettings(settings);
    EXPECT_EQ(Normalized.level, 9);
}

TEST(CacheOptionsTest, NormalizesHashSeed)
{
    beez::core::ContentHashSettings settings;
    settings.algorithm = beez::core::ContentHashAlgorithm::Crc32;
    settings.seed = 42U;

    const auto Normalized = beez::core::normalizeContentHashSettings(settings);
    EXPECT_EQ(Normalized.seed, 42U);
}
