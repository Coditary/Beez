#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
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

TEST(CryptoOpsTest, SupportedAlgorithmsIncludeDigestAndFingerprintHashes)
{
    const auto HashAlgorithms = beez::plugin::lua::crypto_detail::supportedHashAlgorithms();
    EXPECT_GE(HashAlgorithms.size(), 9U);
    EXPECT_TRUE(beez::plugin::lua::crypto_detail::isHashAlgorithm("sha256"));
    EXPECT_TRUE(beez::plugin::lua::crypto_detail::isHashAlgorithm("fnv1a64"));
    EXPECT_TRUE(beez::plugin::lua::crypto_detail::isHashAlgorithm("sdbm"));
    EXPECT_FALSE(beez::plugin::lua::crypto_detail::isHashAlgorithm("unknown"));

    const auto EncodingAlgorithms = beez::plugin::lua::crypto_detail::supportedEncodingAlgorithms();
    ASSERT_EQ(EncodingAlgorithms.size(), 2U);
    EXPECT_TRUE(beez::plugin::lua::crypto_detail::isEncodingAlgorithm("hex"));
    EXPECT_TRUE(beez::plugin::lua::crypto_detail::isEncodingAlgorithm("base64"));
    EXPECT_FALSE(beez::plugin::lua::crypto_detail::isEncodingAlgorithm("sha256"));
}

TEST(CryptoOpsTest, DigestAlgorithmsProduceStableHexDigests)
{
    namespace crypto = beez::plugin::lua::crypto_detail;

    const std::string Sha256 = crypto::hashString("hello", "sha256");
    EXPECT_EQ(Sha256, crypto::hashString("hello", "sha256"));
    EXPECT_EQ(Sha256.size(), 64U);
    EXPECT_NE(Sha256, crypto::hashString("world", "sha256"));

    const std::string Sha512 = crypto::hashString("hello", "sha512");
    EXPECT_EQ(Sha512.size(), 128U);
    EXPECT_EQ(Sha512, crypto::hashString("hello", "sha512"));

    const std::string Sha1 = crypto::hashString("hello", "sha1");
    EXPECT_EQ(Sha1.size(), 40U);
    EXPECT_EQ(Sha1, crypto::hashString("hello", "sha1"));

    const std::string Md5 = crypto::hashString("hello", "md5");
    EXPECT_EQ(Md5.size(), 32U);
    EXPECT_EQ(Md5, crypto::hashString("hello", "md5"));
}

TEST(CryptoOpsTest, FingerprintAlgorithmsProduceStableDigests)
{
    namespace crypto = beez::plugin::lua::crypto_detail;

    for (const char* algorithm : {"fnv1a64", "fnv1a32", "crc32", "djb2", "sdbm"})
    {
        const std::string First = crypto::hashString("beez", algorithm);
        const std::string Second = crypto::hashString("beez", algorithm);
        EXPECT_FALSE(First.empty());
        EXPECT_EQ(First, Second);
        EXPECT_NE(First, crypto::hashString("other", algorithm));
    }
}

TEST(CryptoOpsTest, HashFileMatchesHashString)
{
    namespace crypto = beez::plugin::lua::crypto_detail;

    const auto Directory = std::filesystem::temp_directory_path() / "beez_crypto_ops_test";
    const auto FilePath = Directory / "payload.txt";
    writeFile(FilePath, "hello");

    const std::string FromFile = crypto::hashFile(FilePath, "sha256");
    const std::string FromString = crypto::hashString("hello", "sha256");
    EXPECT_EQ(FromFile, FromString);

    std::filesystem::remove_all(Directory);
}

TEST(CryptoOpsTest, HashFileThrowsWhenMissing)
{
    EXPECT_THROW(
        beez::plugin::lua::crypto_detail::hashFile("/no/such/beez_crypto_file.txt", "sha256"),
        std::runtime_error);
}

TEST(CryptoOpsTest, HashStringRejectsUnknownAlgorithm)
{
    EXPECT_THROW(beez::plugin::lua::crypto_detail::hashString("hello", "nope"), std::runtime_error);
}

TEST(CryptoOpsTest, EncodeHexAndBase64CoverPaddingPaths)
{
    namespace crypto = beez::plugin::lua::crypto_detail;

    EXPECT_EQ(crypto::encodeString("ab", "hex"), "6162");
    EXPECT_EQ(crypto::encodeString("a", "base64"), "YQ==");
    EXPECT_EQ(crypto::encodeString("ab", "base64"), "YWI=");
    EXPECT_EQ(crypto::encodeString("abc", "base64"), "YWJj");
    EXPECT_EQ(crypto::encodeString("abcd", "base64"), "YWJjZA==");
}

TEST(CryptoOpsTest, EncodeRejectsUnknownAlgorithm)
{
    EXPECT_THROW(beez::plugin::lua::crypto_detail::encodeString("hello", "rot13"),
                 std::runtime_error);
}

TEST(CryptoOpsTest, HmacSha256MatchesKnownDigest)
{
    const std::string Digest =
        beez::plugin::lua::crypto_detail::encodeWithKey("hello", "secret", "sha256");
    EXPECT_EQ(Digest, "88aab3ede8d3adf94d26ab90d3bafd4a2083070c3bcce9c014ee04a443847c0b");
}

TEST(CryptoOpsTest, HmacSha512AndLongKeyPaths)
{
    namespace crypto = beez::plugin::lua::crypto_detail;

    const std::string Sha512Digest = crypto::encodeWithKey("hello", "secret", "sha512");
    EXPECT_EQ(Sha512Digest.size(), 128U);
    EXPECT_EQ(Sha512Digest, crypto::encodeWithKey("hello", "secret", "sha512"));

    const std::string LongKey(100U, 'x');
    const std::string LongKeyDigest = crypto::encodeWithKey("hello", LongKey, "sha256");
    EXPECT_EQ(LongKeyDigest.size(), 64U);
    EXPECT_EQ(LongKeyDigest, crypto::encodeWithKey("hello", LongKey, "sha256"));
}

TEST(CryptoOpsTest, HmacRejectsFingerprintAlgorithms)
{
    EXPECT_THROW(beez::plugin::lua::crypto_detail::encodeWithKey("hello", "key", "sdbm"),
                 std::runtime_error);
}
