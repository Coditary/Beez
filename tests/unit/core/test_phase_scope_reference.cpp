#include "beez/core/model/phase_scope_reference.hpp"

#include <gtest/gtest.h>

TEST(PhaseScopeReferenceTest, ParsesColonAndBracketForms)
{
    const auto Colon = beez::core::parseWorkflowPhaseReference("configure:release");
    EXPECT_EQ(Colon.phase, "configure");
    EXPECT_EQ(Colon.scope, "release");

    const auto Bracket = beez::core::parseWorkflowPhaseReference("build[code]");
    EXPECT_EQ(Bracket.phase, "build");
    EXPECT_EQ(Bracket.scope, "code");
}

TEST(PhaseScopeReferenceTest, ParsesUnscopedWorkflowPhaseReference)
{
    const auto Unscoped = beez::core::parseWorkflowPhaseReference("setup");
    EXPECT_EQ(Unscoped.phase, "setup");
    EXPECT_TRUE(Unscoped.scope.empty());
}

TEST(PhaseScopeReferenceTest, ParsesScopedReference)
{
    const auto Unscoped = beez::core::parseScopedReference("configure");
    EXPECT_EQ(Unscoped.name, "configure");
    EXPECT_TRUE(Unscoped.scopes.empty());

    const auto Single = beez::core::parseScopedReference("configure[debug]");
    EXPECT_EQ(Single.name, "configure");
    ASSERT_EQ(Single.scopes.size(), 1U);
    EXPECT_EQ(Single.scopes[0], "debug");

    const auto Multiple = beez::core::parseScopedReference(R"(compile["debug", "fuzzer"])");
    EXPECT_EQ(Multiple.name, "compile");
    ASSERT_EQ(Multiple.scopes.size(), 2U);
    EXPECT_EQ(Multiple.scopes[0], "debug");
    EXPECT_EQ(Multiple.scopes[1], "fuzzer");
}
