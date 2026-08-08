#pragma once

#include "beez/core/cache_options.hpp"

#include <memory>
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

}  // namespace beez::core
