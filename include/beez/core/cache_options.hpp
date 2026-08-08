#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace beez::core
{

constexpr int DefaultCacheCompressionLevel = 6;
constexpr int MaxCacheCompressionLevel = 9;
constexpr std::size_t DefaultMmapHashingMinBytes = 65536;

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
    Delta,
    VByte,
};

enum class CacheCompressionMode : std::uint8_t
{
    Never,
    Always,
    Auto,
};

class CacheWriteCoordinator;

struct ContentHashSettings
{
    ContentHashAlgorithm algorithm = ContentHashAlgorithm::Fnv1a64;
    std::uint32_t seed = 0;
    bool useMmapForHashing = false;
    std::size_t mmapHashingMinBytes = DefaultMmapHashingMinBytes;
};

struct CacheCompressionSettings
{
    CacheCompressionAlgorithm algorithm = CacheCompressionAlgorithm::None;
    int level = DefaultCacheCompressionLevel;
    CacheCompressionMode mode = CacheCompressionMode::Auto;
};

struct CacheOptions
{
    bool enabled = true;
    std::filesystem::path root;
    bool protect = false;
    ContentHashSettings hash;
    CacheCompressionSettings compress;
    CacheWriteCoordinator* writeCoordinator = nullptr;
};

[[nodiscard]] ContentHashAlgorithm parseContentHashAlgorithm(const std::string& value);
[[nodiscard]] const char* toString(ContentHashAlgorithm algorithm);
[[nodiscard]] std::vector<const char*> contentHashAlgorithmNames();

[[nodiscard]] CacheCompressionAlgorithm parseCacheCompressionAlgorithm(const std::string& value);
[[nodiscard]] const char* toString(CacheCompressionAlgorithm algorithm);
[[nodiscard]] std::vector<const char*> cacheCompressionAlgorithmNames();

[[nodiscard]] CacheCompressionMode parseCacheCompressionMode(const std::string& value);
[[nodiscard]] const char* toString(CacheCompressionMode mode);
[[nodiscard]] std::vector<const char*> cacheCompressionModeNames();

[[nodiscard]] ContentHashSettings normalizeContentHashSettings(const ContentHashSettings& settings);
[[nodiscard]] CacheCompressionSettings
normalizeCacheCompressionSettings(CacheCompressionSettings settings);

}  // namespace beez::core
