#include "beez/core/phase_argument_parser.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

TEST(PhaseArgumentParserTest, ParsesPhaseWithoutScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->phase, "generate");
    EXPECT_TRUE(Parsed->scopes.empty());
}

TEST(PhaseArgumentParserTest, ParsesColonSyntaxWithSingleScope)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate:code");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->phase, "generate");
    ASSERT_EQ(Parsed->scopes.size(), 1U);
    EXPECT_EQ(Parsed->scopes[0], "code");
}

TEST(PhaseArgumentParserTest, ParsesColonSyntaxWithMultipleScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument("generate:code,docs");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->phase, "generate");
    ASSERT_EQ(Parsed->scopes.size(), 2U);
    EXPECT_EQ(Parsed->scopes[0], "code");
    EXPECT_EQ(Parsed->scopes[1], "docs");
}

TEST(PhaseArgumentParserTest, ParsesBracketSyntaxWithQuotedScopes)
{
    const auto Parsed = beez::core::parsePhaseArgument(R"(generate["code","docs"])");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->phase, "generate");
    ASSERT_EQ(Parsed->scopes.size(), 2U);
    EXPECT_EQ(Parsed->scopes[0], "code");
    EXPECT_EQ(Parsed->scopes[1], "docs");
}

TEST(PhaseArgumentParserTest, ReturnsEmptyForInvalidColonSyntax)
{
    EXPECT_FALSE(beez::core::parsePhaseArgument("").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument(":code").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument("generate:").has_value());
    EXPECT_FALSE(beez::core::parsePhaseArgument("generate:code,").has_value());
}
