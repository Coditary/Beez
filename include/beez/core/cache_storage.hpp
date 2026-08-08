#pragma once

#include "beez/core/cache_options.hpp"

#include <filesystem>
#include <string>

namespace beez::core
{

void writeCacheFile(const std::filesystem::path& path,
                    std::string content,
                    const CacheOptions& options);

[[nodiscard]] std::string readCacheFile(const std::filesystem::path& path,
                                        const CacheOptions& options);

void prepareCacheFileForWrite(const std::filesystem::path& path, bool protect);

void applyCacheFileProtection(const std::filesystem::path& path, bool protect);

}  // namespace beez::core
