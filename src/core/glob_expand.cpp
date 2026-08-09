#include "beez/core/glob_expand.hpp"

#include "beez/core/glob_metadata_cache.hpp"
#include "beez/core/glob_pattern.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] bool shouldSkipPath(const std::filesystem::path& path)
{
    return std::ranges::any_of(path,
                               [](const auto& part)
                               {
                                   auto partName = part.string();
                                   return partName == ".git" || partName == ".cache";
                               });
}

[[nodiscard]] std::string relativePath(const std::filesystem::path& projectRoot,
                                       const std::filesystem::path& absolutePath)
{
    return absolutePath.lexically_relative(projectRoot).generic_string();
}

[[nodiscard]] bool hasWildcard(const std::string& pattern)
{
    return pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos;
}

[[nodiscard]] std::size_t indexOfFirstWildcard(const std::string& pattern)
{
    const auto Wildcard = std::ranges::find_if(
        pattern, [](const char Character) { return Character == '*' || Character == '?'; });
    return static_cast<std::size_t>(std::distance(pattern.begin(), Wildcard));
}

[[nodiscard]] std::filesystem::path searchRootForPattern(const std::string& pattern,
                                                         const std::filesystem::path& projectRoot)
{
    const std::size_t WildcardIndex = indexOfFirstWildcard(pattern);
    if (WildcardIndex == pattern.size())
    {
        return projectRoot;
    }

    const std::string Prefix = pattern.substr(0, WildcardIndex);
    const auto Slash = Prefix.rfind('/');
    if (Slash == std::string::npos)
    {
        return projectRoot;
    }

    return projectRoot / Prefix.substr(0, Slash);
}

void collectLiteralMatch(const std::string& pattern,
                         const std::filesystem::path& projectRoot,
                         std::vector<std::string>& matches)
{
    const auto Absolute = projectRoot / pattern;
    if (!std::filesystem::is_regular_file(Absolute))
    {
        return;
    }

    matches.push_back(pattern);
}

void collectMatches(const std::string& pattern,
                    const std::filesystem::path& projectRoot,
                    const IGlobMatcher& matcher,
                    std::vector<std::string>& matches)
{
    if (!hasWildcard(pattern))
    {
        collectLiteralMatch(pattern, projectRoot, matches);
        return;
    }

    const auto SearchRoot = searchRootForPattern(pattern, projectRoot);
    if (!std::filesystem::exists(SearchRoot))
    {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             SearchRoot, std::filesystem::directory_options::skip_permission_denied))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto Relative = relativePath(projectRoot, entry.path());
        if (Relative.empty() || Relative.starts_with("../") || shouldSkipPath(entry.path()))
        {
            continue;
        }

        if (matcher.matches(pattern, Relative))
        {
            matches.push_back(Relative);
        }
    }
}

}  // namespace

std::vector<std::string> expandGlobPatterns(const std::vector<std::string>& patterns,
                                            const std::filesystem::path& projectRoot,
                                            const IGlobMatcher& matcher,
                                            GlobMetadataCache* metadataCache)
{
    std::vector<std::string> matches;
    for (const auto& pattern : patterns)
    {
        if (metadataCache != nullptr)
        {
            if (const auto Cached = metadataCache->lookup(pattern, projectRoot); Cached.has_value())
            {
                matches.insert(matches.end(), Cached->begin(), Cached->end());
                continue;
            }
        }

        std::vector<std::string> patternMatches;
        collectMatches(pattern, projectRoot, matcher, patternMatches);

        if (metadataCache != nullptr)
        {
            metadataCache->store(pattern, projectRoot, patternMatches);
        }

        matches.insert(matches.end(), patternMatches.begin(), patternMatches.end());
    }

    std::ranges::sort(matches);
    auto uniqueEnd = std::ranges::unique(matches).begin();
    matches.erase(uniqueEnd, matches.end());
    return matches;
}

}  // namespace beez::core
