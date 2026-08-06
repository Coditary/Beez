#pragma once

#include <memory>
#include <string>

namespace beez::core
{

// Abstraction for glob matching so the implementation can be swapped later
// (e.g. indexed/trie-based matching for large repos).
class IGlobMatcher
{
  public:
    IGlobMatcher() = default;
    virtual ~IGlobMatcher() = default;

    IGlobMatcher(const IGlobMatcher&) = delete;
    IGlobMatcher& operator=(const IGlobMatcher&) = delete;
    IGlobMatcher(IGlobMatcher&&) = delete;
    IGlobMatcher& operator=(IGlobMatcher&&) = delete;

    [[nodiscard]] virtual bool matches(const std::string& pattern,
                                       const std::string& path) const = 0;

    [[nodiscard]] virtual bool patternsOverlap(const std::string& leftPattern,
                                               const std::string& rightPattern) const = 0;
};

[[nodiscard]] std::unique_ptr<IGlobMatcher> makeSimpleGlobMatcher();

}  // namespace beez::core
