#pragma once

#include "beez/core/cache/fingerprint/content_hash.hpp"

#include <filesystem>
#include <string>

namespace beez::core
{

[[nodiscard]] std::string includeTreeFingerprint(const std::filesystem::path& sourcePath,
                                                 const std::filesystem::path& projectRoot,
                                                 const IContentHasher& hasher);

}  // namespace beez::core
