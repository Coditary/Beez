#pragma once

#include "beez/core/cache_options.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace beez::core
{

void writeCacheFile(const std::filesystem::path& path,
                    const std::string& content,
                    const CacheOptions& options);

[[nodiscard]] std::string readCacheFile(const std::filesystem::path& path,
                                        const CacheOptions& options);

void prepareCacheFileForWrite(const std::filesystem::path& path, bool protect);

void applyCacheFileProtection(const std::filesystem::path& path, bool protect);

// Applies configuration-driven cache storage updates (e.g. recompress on-disk envelopes).
[[nodiscard]] std::size_t updateCacheStorage(const CacheOptions& options);

}  // namespace beez::core
