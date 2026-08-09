#include "beez/core/cache/compress.hpp"
#include "beez/core/config/cache_options.hpp"

#include <gtest/gtest.h>

#include <cstddef>
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

TEST(CacheCompressTest, GzipRoundTripsLargePayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Gzip;
    settings.level = 6;

    std::string payload;
    payload.reserve(20'000U);
    for (std::size_t index = 0; index < 200U; ++index)
    {
        payload += "input=src/file_" + std::to_string(index) + ".cpp\t12345\t67890\n";
    }

    EXPECT_EQ(roundTrip(settings, payload), payload);
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

TEST(CacheCompressTest, DeltaRoundTripsMonotonicPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Delta;

    std::string payload;
    for (int value = 0; value < 256; ++value)
    {
        payload.push_back(static_cast<char>(value));
    }

    EXPECT_EQ(roundTrip(settings, payload), payload);
}

TEST(CacheCompressTest, VByteRoundTripsTextPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::VByte;

    const std::string Payload = "step=compile\noutput=build/app.o\n";
    EXPECT_EQ(roundTrip(settings, Payload), Payload);
}

TEST(CacheCompressTest, VByteRoundTripsEmptyPayload)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::VByte;

    EXPECT_TRUE(roundTrip(settings, "").empty());
}

TEST(CacheCompressTest, EstimateMatchesActualForRle)
{
    beez::core::CacheCompressionSettings settings;
    settings.algorithm = beez::core::CacheCompressionAlgorithm::Rle;

    const std::string Payload(128, 'a');
    const auto Compressor = beez::core::makeCacheCompressor(settings);
    const auto Estimate = beez::core::estimateCacheCompressedBodySize(settings.algorithm, Payload);
    ASSERT_TRUE(Estimate.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate
    EXPECT_EQ(Estimate.value(), Compressor->compress(Payload).size());
}

TEST(CacheCompressTest, ZlibMightHelpRejectsTinyPayload)
{
    const std::string Payload = "step=qa:security-tidy\n";
    EXPECT_FALSE(beez::core::zlibCompressionMightHelp(Payload, 40U));
}
