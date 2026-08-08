#include "beez/core/cache_options.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string toLower(std::string value)
{
    std::ranges::transform(value,
                           value.begin(),
                           [](const unsigned char Character)
                           { return static_cast<char>(std::tolower(Character)); });
    return value;
}

}  // namespace

ContentHashAlgorithm parseContentHashAlgorithm(const std::string& value)
{
    const std::string Normalized = toLower(value);
    if (Normalized == "fnv1a64")
    {
        return ContentHashAlgorithm::Fnv1a64;
    }
    if (Normalized == "fnv1a32")
    {
        return ContentHashAlgorithm::Fnv1a32;
    }
    if (Normalized == "crc32")
    {
        return ContentHashAlgorithm::Crc32;
    }
    if (Normalized == "djb2")
    {
        return ContentHashAlgorithm::Djb2;
    }
    if (Normalized == "sdbm")
    {
        return ContentHashAlgorithm::Sdbm;
    }

    throw std::runtime_error(
        "cache.hash.algorithm must be one of: fnv1a64, fnv1a32, crc32, djb2, sdbm");
}

const char* toString(ContentHashAlgorithm algorithm)
{
    switch (algorithm)
    {
    case ContentHashAlgorithm::Fnv1a64:
        return "fnv1a64";
    case ContentHashAlgorithm::Fnv1a32:
        return "fnv1a32";
    case ContentHashAlgorithm::Crc32:
        return "crc32";
    case ContentHashAlgorithm::Djb2:
        return "djb2";
    case ContentHashAlgorithm::Sdbm:
        return "sdbm";
    }
    return "fnv1a64";
}

CacheCompressionAlgorithm parseCacheCompressionAlgorithm(const std::string& value)
{
    const std::string Normalized = toLower(value);
    if (Normalized == "none")
    {
        return CacheCompressionAlgorithm::None;
    }
    if (Normalized == "gzip")
    {
        return CacheCompressionAlgorithm::Gzip;
    }
    if (Normalized == "zlib")
    {
        return CacheCompressionAlgorithm::Zlib;
    }
    if (Normalized == "rle")
    {
        return CacheCompressionAlgorithm::Rle;
    }
    if (Normalized == "deflate")
    {
        return CacheCompressionAlgorithm::Deflate;
    }

    throw std::runtime_error(
        "cache.compress.algorithm must be one of: none, gzip, zlib, rle, deflate");
}

const char* toString(CacheCompressionAlgorithm algorithm)
{
    switch (algorithm)
    {
    case CacheCompressionAlgorithm::None:
        return "none";
    case CacheCompressionAlgorithm::Gzip:
        return "gzip";
    case CacheCompressionAlgorithm::Zlib:
        return "zlib";
    case CacheCompressionAlgorithm::Rle:
        return "rle";
    case CacheCompressionAlgorithm::Deflate:
        return "deflate";
    }
    return "none";
}

ContentHashSettings normalizeContentHashSettings(ContentHashSettings settings)
{
    return settings;
}

CacheCompressionSettings normalizeCacheCompressionSettings(CacheCompressionSettings settings)
{
    settings.level = std::clamp(settings.level, 0, MaxCacheCompressionLevel);
    return settings;
}

}  // namespace beez::core
