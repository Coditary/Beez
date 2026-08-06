#include "beez/core/glob_pattern.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace
{

class GlobPatternTest : public ::testing::Test
{
  protected:
    std::unique_ptr<beez::core::IGlobMatcher> matcher = beez::core::makeSimpleGlobMatcher();
};

}  // namespace

TEST_F(GlobPatternTest, MatchesExactPath)
{
    EXPECT_TRUE(matcher->matches("src/main.cpp", "src/main.cpp"));
    EXPECT_FALSE(matcher->matches("src/main.cpp", "src/other.cpp"));
}

TEST_F(GlobPatternTest, MatchesSingleStarGlob)
{
    EXPECT_TRUE(matcher->matches("src/*.cpp", "src/main.cpp"));
    EXPECT_FALSE(matcher->matches("src/*.cpp", "src/nested/main.cpp"));
}

TEST_F(GlobPatternTest, MatchesDoubleStarGlob)
{
    EXPECT_TRUE(matcher->matches("src/**/*.cpp", "src/nested/main.cpp"));
    EXPECT_TRUE(matcher->matches("src/**/*.cpp", "src/main.cpp"));
    EXPECT_FALSE(matcher->matches("src/**/*.cpp", "lib/main.cpp"));
}

TEST_F(GlobPatternTest, PatternsOverlapWhenTheyShareFiles)
{
    EXPECT_TRUE(matcher->patternsOverlap("src/**/*.cpp", "src/**/main.cpp"));
    EXPECT_TRUE(matcher->patternsOverlap("build/**/*.o", "build/**/*.o"));
}

TEST_F(GlobPatternTest, PatternsDoNotOverlapWhenDisjoint)
{
    EXPECT_FALSE(matcher->patternsOverlap("src/**/*.cpp", "build/**/*.o"));
    EXPECT_FALSE(matcher->patternsOverlap("docs/**", "src/**"));
}
