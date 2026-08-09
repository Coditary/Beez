#pragma once

#include "beez/core/config/cache_options.hpp"

#include <filesystem>
#include <string>

namespace beez::core::storage_detail
{

[[nodiscard]] std::string readBinaryFile(const std::filesystem::path& path);

void writeBinaryFile(const std::filesystem::path& path, const std::string& content);

[[nodiscard]] std::string buildCachePayload(const std::string& content,
                                            const CacheCompressionSettings& settings);

}  // namespace beez::core::storage_detail
