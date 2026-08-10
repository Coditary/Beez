#pragma once

#include "beez/core/config/cache/cache_options.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace beez::core
{

class ICacheCompressor
{
  public:
    ICacheCompressor() = default;
    virtual ~ICacheCompressor() = default;

    ICacheCompressor(const ICacheCompressor&) = delete;
    ICacheCompressor& operator=(const ICacheCompressor&) = delete;
    ICacheCompressor(ICacheCompressor&&) = delete;
    ICacheCompressor& operator=(ICacheCompressor&&) = delete;

    [[nodiscard]] virtual std::string compress(std::string_view data) const = 0;
    [[nodiscard]] virtual std::string decompress(std::string_view data) const = 0;
};

[[nodiscard]] std::unique_ptr<ICacheCompressor>
makeCacheCompressor(const CacheCompressionSettings& settings);

// Exact compressed-body size for non-zlib algorithms. Returns nullopt when a trial
// compression is required (gzip/zlib/deflate).
[[nodiscard]] std::optional<std::size_t>
estimateCacheCompressedBodySize(CacheCompressionAlgorithm algorithm, std::string_view data);

// Cheap pre-check for zlib-family auto mode before running deflate.
[[nodiscard]] bool zlibCompressionMightHelp(std::string_view data, std::size_t envelopeHeaderSize);

}  // namespace beez::core
