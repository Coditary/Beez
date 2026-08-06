#include "beez/core/glob_pattern.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>

namespace beez::core
{

namespace
{

std::string globToRegex(const std::string& pattern)
{
    std::string regex = "^";
    for (std::size_t index = 0; index < pattern.size(); ++index)
    {
        const char Character = pattern.at(index);
        if (Character == '*')
        {
            if (index + 1 < pattern.size() && pattern.at(index + 1) == '*')
            {
                regex += ".*";
                ++index;
                if (index + 1 < pattern.size() && pattern.at(index + 1) == '/')
                {
                    ++index;
                }
            }
            else
            {
                regex += "[^/]*";
            }
            continue;
        }

        if (Character == '?')
        {
            regex += "[^/]";
            continue;
        }

        if (Character == '.' || Character == '+' || Character == '(' || Character == ')' ||
            Character == '|' || Character == '^' || Character == '$' || Character == '{' ||
            Character == '}' || Character == '[' || Character == ']' || Character == '\\')
        {
            regex += '\\';
        }
        regex += Character;
    }
    regex += "$";
    return regex;
}

std::string concreteSample(const std::string& pattern)
{
    std::string sample;
    for (std::size_t index = 0; index < pattern.size(); ++index)
    {
        const char Character = pattern.at(index);
        if (Character == '*' && index + 1 < pattern.size() && pattern.at(index + 1) == '*')
        {
            sample += "/x";
            ++index;
            if (index + 1 < pattern.size() && pattern.at(index + 1) == '/')
            {
                ++index;
            }
            continue;
        }

        if (Character == '*')
        {
            sample += 'x';
            continue;
        }

        if (Character == '?')
        {
            sample += 'a';
            continue;
        }

        sample += Character;
    }
    return sample;
}

[[nodiscard]] std::size_t firstWildcardIndex(const std::string& pattern)
{
    const auto Wildcard = std::ranges::find_if(
        pattern, [](const char Character) { return Character == '*' || Character == '?'; });
    return static_cast<std::size_t>(std::distance(pattern.begin(), Wildcard));
}

[[nodiscard]] bool literalPrefixMayOverlap(const std::string& leftPattern,
                                           const std::string& rightPattern)
{
    if (leftPattern == rightPattern)
    {
        return true;
    }

    const std::string LeftLiteral = leftPattern.substr(0, firstWildcardIndex(leftPattern));
    const std::string RightLiteral = rightPattern.substr(0, firstWildcardIndex(rightPattern));

    if (LeftLiteral == RightLiteral)
    {
        return true;
    }

    if (LeftLiteral.starts_with(RightLiteral) || RightLiteral.starts_with(LeftLiteral))
    {
        return true;
    }

    return LeftLiteral.empty() || RightLiteral.empty();
}

using PatternPairKey = std::string;

[[nodiscard]] PatternPairKey overlapCacheKey(const std::string& left, const std::string& right)
{
    if (left < right)
    {
        return left + '\x1f' + right;
    }
    return right + '\x1f' + left;
}

class CachedGlobMatcher final : public IGlobMatcher
{
  public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- pattern/path order is part of API
    [[nodiscard]] bool matches(const std::string& pattern, const std::string& path) const override
    {
        return std::regex_match(path, compiledRegex(pattern));
    }

    [[nodiscard]] bool patternsOverlap(const std::string& leftPattern,
                                       const std::string& rightPattern) const override
    {
        if (leftPattern == rightPattern)
        {
            return true;
        }

        if (!literalPrefixMayOverlap(leftPattern, rightPattern))
        {
            return false;
        }

        const auto CacheKey = overlapCacheKey(leftPattern, rightPattern);
        const auto Cached = overlapCache_.find(CacheKey);
        if (Cached != overlapCache_.end())
        {
            return Cached->second;
        }

        const std::regex& leftRegex = compiledRegex(leftPattern);
        const std::regex& rightRegex = compiledRegex(rightPattern);
        const std::string& leftSample = concreteSampleCached(leftPattern);
        const std::string& rightSample = concreteSampleCached(rightPattern);
        const bool Overlaps =
            std::regex_match(leftSample, rightRegex) || std::regex_match(rightSample, leftRegex);
        overlapCache_.emplace(CacheKey, Overlaps);
        return Overlaps;
    }

  private:
    [[nodiscard]] const std::regex& compiledRegex(const std::string& pattern) const
    {
        const auto Found = regexCache_.find(pattern);
        if (Found != regexCache_.end())
        {
            return *Found->second;
        }

        const auto Inserted =
            regexCache_.emplace(pattern, std::make_shared<std::regex>(globToRegex(pattern)));
        return *Inserted.first->second;
    }

    [[nodiscard]] const std::string& concreteSampleCached(const std::string& pattern) const
    {
        const auto Found = sampleCache_.find(pattern);
        if (Found != sampleCache_.end())
        {
            return Found->second;
        }

        const auto Inserted = sampleCache_.emplace(pattern, concreteSample(pattern));
        return Inserted.first->second;
    }

    mutable std::unordered_map<std::string, std::shared_ptr<std::regex>> regexCache_;
    mutable std::unordered_map<std::string, std::string> sampleCache_;
    mutable std::unordered_map<PatternPairKey, bool> overlapCache_;
};

}  // namespace

std::unique_ptr<IGlobMatcher> makeSimpleGlobMatcher()
{
    return std::make_unique<CachedGlobMatcher>();
}

IGlobMatcher& defaultGlobMatcher()
{
    static CachedGlobMatcher matcher;
    return matcher;
}

}  // namespace beez::core
