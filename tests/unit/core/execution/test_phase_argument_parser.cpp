#include "beez/core/execution/phase_argument_parser.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

// NOLINTBEGIN(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate to
// clang-tidy
TEST(PhaseArgumentParserTest, ParsesPhaseWithoutScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate");
    ASSERT_TRUE(Parsed.has_value());
    const auto& parsed = Parsed.value();
    EXPECT_EQ(parsed.phase, "generate");
    EXPECT_TRUE(parsed.scopes.empty());
}

TEST(PhaseArgumentParserTest, ParsesColonSyntaxWithSingleScope)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate:code");
    ASSERT_TRUE(Parsed.has_value());
    const auto& parsed = Parsed.value();
    EXPECT_EQ(parsed.phase, "generate");
    ASSERT_EQ(parsed.scopes.size(), 1U);
    EXPECT_EQ(parsed.scopes[0], "code");
}

TEST(PhaseArgumentParserTest, ParsesColonSyntaxWithMultipleScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate:code,docs");
    ASSERT_TRUE(Parsed.has_value());
    const auto& parsed = Parsed.value();
    EXPECT_EQ(parsed.phase, "generate");
    ASSERT_EQ(parsed.scopes.size(), 2U);
    EXPECT_EQ(parsed.scopes[0], "code");
    EXPECT_EQ(parsed.scopes[1], "docs");
}

TEST(PhaseArgumentParserTest, ParsesBracketSyntaxWithQuotedScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument(R"(generate["code","docs"])");
    ASSERT_TRUE(Parsed.has_value());
    const auto& parsed = Parsed.value();
    EXPECT_EQ(parsed.phase, "generate");
    ASSERT_EQ(parsed.scopes.size(), 2U);
    EXPECT_EQ(parsed.scopes[0], "code");
    EXPECT_EQ(parsed.scopes[1], "docs");
}

TEST(PhaseArgumentParserTest, ReturnsEmptyForInvalidColonSyntax)
{
    EXPECT_FALSE(beez::core::parsePhaseArgument("").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument(":code").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument("generate:").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument("generate:code,").has_value());
}

// NOLINTEND(bugprone-unchecked-optional-access)