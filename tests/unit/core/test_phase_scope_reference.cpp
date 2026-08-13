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
