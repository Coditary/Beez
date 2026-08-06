#include "beez/core/glob_pattern.hpp"

#include <cstddef>
#include <memory>
#include <regex>
#include <string>

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

class SimpleGlobMatcher final : public IGlobMatcher
{
  public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- pattern/path order is part of API
    [[nodiscard]] bool matches(const std::string& pattern, const std::string& path) const override
    {
        const std::regex PatternRegex(globToRegex(pattern));
        return std::regex_match(path, PatternRegex);
    }

    [[nodiscard]] bool patternsOverlap(const std::string& leftPattern,
                                       const std::string& rightPattern) const override
    {
        if (leftPattern == rightPattern)
        {
            return true;
        }

        const std::regex LeftRegex(globToRegex(leftPattern));
        const std::regex RightRegex(globToRegex(rightPattern));
        const std::string LeftSample = concreteSample(leftPattern);
        const std::string RightSample = concreteSample(rightPattern);
        return std::regex_match(LeftSample, RightRegex) || std::regex_match(RightSample, LeftRegex);
    }
};

}  // namespace

std::unique_ptr<IGlobMatcher> makeSimpleGlobMatcher()
{
    return std::make_unique<SimpleGlobMatcher>();
}

}  // namespace beez::core
