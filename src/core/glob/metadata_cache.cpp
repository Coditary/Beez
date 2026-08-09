#include "beez/core/glob/metadata_cache.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

GlobMetadataCache::GlobMetadataCache(bool enabled) : enabled_(enabled) {}

void GlobMetadataCache::clear()
{
    const std::scoped_lock Lock(mutex_);
    entries_.clear();
}

std::optional<std::vector<std::string>>
GlobMetadataCache::lookup(const std::string& pattern,
                          const std::filesystem::path& projectRoot) const
{
    if (!enabled_)
    {
        return std::nullopt;
    }

    const std::scoped_lock Lock(mutex_);
    const auto Found = entries_.find(cacheKey(pattern, projectRoot));
    if (Found == entries_.end())
    {
        return std::nullopt;
    }

    return Found->second;
}

void GlobMetadataCache::store(const std::string& pattern,
                              const std::filesystem::path& projectRoot,
                              std::vector<std::string> matches)
{
    if (!enabled_)
    {
        return;
    }

    const std::scoped_lock Lock(mutex_);
    entries_[cacheKey(pattern, projectRoot)] = std::move(matches);
}

std::string GlobMetadataCache::cacheKey(const std::string& pattern,
                                        const std::filesystem::path& projectRoot)
{
    return projectRoot.lexically_normal().generic_string() + '\0' + pattern;
}

}  // namespace beez::core
