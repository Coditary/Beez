#pragma once

#include "beez/core/config/cache_options.hpp"

#include <cstddef>

namespace beez::core
{

// Applies configuration-driven cache storage updates (e.g. recompress on-disk envelopes).
[[nodiscard]] std::size_t updateCacheStorage(const CacheOptions& options);

}  // namespace beez::core
