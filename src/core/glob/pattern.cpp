#include "beez/core/glob/pattern.hpp"

#include <coditary/utils/glob.hpp>
#include <memory>

namespace beez::core
{

namespace
{

class GlobMatcherAdapter final : public IGlobMatcher
{
  public:
    [[nodiscard]] bool matches(const std::string& pattern, const std::string& path) const override
    {
        return coditary::utils::default_glob_matcher().matches(pattern, path);
    }

    [[nodiscard]] bool patternsOverlap(const std::string& leftPattern,
                                       const std::string& rightPattern) const override
    {
        return coditary::utils::default_glob_matcher().patterns_overlap(leftPattern, rightPattern);
    }
};

}  // namespace

std::unique_ptr<IGlobMatcher> makeSimpleGlobMatcher()
{
    return std::make_unique<GlobMatcherAdapter>();
}

IGlobMatcher& defaultGlobMatcher()
{
    static GlobMatcherAdapter matcher;
    return matcher;
}

}  // namespace beez::core
