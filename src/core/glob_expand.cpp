#include "beez/core/glob_expand.hpp"

#include "beez/core/glob_pattern.hpp"

#include <algorithm>
#include <filesystem>
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

void collectMatches(const std::string& pattern,
                    const std::filesystem::path& projectRoot,
                    const IGlobMatcher& matcher,
                    std::vector<std::string>& matches)
{
    if (!std::filesystem::exists(projectRoot))
    {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             projectRoot, std::filesystem::directory_options::skip_permission_denied))
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
                                            const IGlobMatcher& matcher)
{
    std::vector<std::string> matches;
    for (const auto& pattern : patterns)
    {
        collectMatches(pattern, projectRoot, matcher, matches);
    }

    std::ranges::sort(matches);
    auto uniqueEnd = std::ranges::unique(matches).begin();
    matches.erase(uniqueEnd, matches.end());
    return matches;
}

}  // namespace beez::core
