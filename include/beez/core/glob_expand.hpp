#pragma once

#include "beez/core/glob_pattern.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace beez::core
{

class GlobMetadataCache;

[[nodiscard]] std::vector<std::string>
expandGlobPatterns(const std::vector<std::string>& patterns,
                   const std::filesystem::path& projectRoot,
                   const IGlobMatcher& matcher,
                   GlobMetadataCache* metadataCache = nullptr);

}  // namespace beez::core
