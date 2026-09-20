#include "beez/core/glob/expand.hpp"

#include "beez/core/glob/metadata_cache.hpp"
#include "beez/core/glob/pattern.hpp"

#include <coditary/fs/glob_walk.hpp>
#include <coditary/utils/glob.hpp>

namespace beez::core
{

std::vector<std::string> expandGlobPatterns(const std::vector<std::string>& patterns,
                                            const std::filesystem::path& projectRoot,
                                            const IGlobMatcher& /*matcher*/,
                                            GlobMetadataCache* /*metadataCache*/)
{
    return coditary::fs::expand_glob_patterns(
        patterns, projectRoot, coditary::utils::default_glob_matcher(), nullptr);
}

}  // namespace beez::core
