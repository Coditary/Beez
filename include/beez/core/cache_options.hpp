#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace beez::core
{

constexpr int DefaultCacheCompressionLevel = 6;
constexpr int MaxCacheCompressionLevel = 9;

enum class ContentHashAlgorithm : std::uint8_t
{
    Fnv1a64,
    Fnv1a32,
    Crc32,
    Djb2,
    Sdbm,
};

enum class CacheCompressionAlgorithm : std::uint8_t
{
    None,
    Gzip,
    Zlib,
    Rle,
    Deflate,
};

struct ContentHashSettings
{
    ContentHashAlgorithm algorithm = ContentHashAlgorithm::Fnv1a64;
    std::uint32_t seed = 0;
};

struct CacheCompressionSettings
{
    CacheCompressionAlgorithm algorithm = CacheCompressionAlgorithm::None;
    int level = DefaultCacheCompressionLevel;
};

struct CacheOptions
{
    bool enabled = true;
    std::filesystem::path root;
    bool protect = false;
    ContentHashSettings hash;
    CacheCompressionSettings compress;
};

[[nodiscard]] ContentHashAlgorithm parseContentHashAlgorithm(const std::string& value);
[[nodiscard]] const char* toString(ContentHashAlgorithm algorithm);
[[nodiscard]] std::vector<const char*> contentHashAlgorithmNames();

[[nodiscard]] CacheCompressionAlgorithm parseCacheCompressionAlgorithm(const std::string& value);
[[nodiscard]] const char* toString(CacheCompressionAlgorithm algorithm);
[[nodiscard]] std::vector<const char*> cacheCompressionAlgorithmNames();

[[nodiscard]] ContentHashSettings normalizeContentHashSettings(ContentHashSettings settings);
[[nodiscard]] CacheCompressionSettings
normalizeCacheCompressionSettings(CacheCompressionSettings settings);

}  // namespace beez::core
