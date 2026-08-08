#include "beez/core/cache_compress.hpp"
#include "beez/core/cache_options.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{

[[nodiscard]] std::string roundTrip(const beez::core::CacheCompressionSettings& settings,
                                    const std::string& input)
{
    const auto Compressor = beez::core::makeCacheCompressor(settings);
    return Compressor->decompress(Compressor->compress(input));
}

}  // namespace

TEST(CacheCompressTest, NonePassesThroughUnchanged)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::None;

    const auto Compressor = beez::core::makeCacheCompressor(settings);
    const std::string Payload = "manifest payload";
    EXPECT_EQ(Compressor->compress(Payload), Payload);
    EXPECT_EQ(Compressor->decompress(Payload), Payload);
}

TEST(CacheCompressTest, RleRoundTripsRepetitiveData)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Rle;

    const std::string Payload(128, 'a');
    EXPECT_EQ(roundTrip(settings, Payload), Payload);
}

TEST(CacheCompressTest, GzipRoundTripsTextPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    settings.level = 6;

    const std::string Payload = "step=compile\noutput=build/app.o\n";
    EXPECT_EQ(roundTrip(settings, Payload), Payload);
}

TEST(CacheCompressTest, ZlibRoundTripsTextPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Zlib;
    settings.level = 3;

    const std::string Payload = "config_hash=abc\nversion=0.1.0\n";
    EXPECT_EQ(roundTrip(settings, Payload), Payload);
}

TEST(CacheCompressTest, DeflateRoundTripsTextPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Deflate;
    settings.level = 1;

    const std::string Payload = "kind=file\nkey=src/main.cpp\n";
    EXPECT_EQ(roundTrip(settings, Payload), Payload);
}
