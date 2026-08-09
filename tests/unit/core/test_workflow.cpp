#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <gtest/gtest.h>

TEST(WorkflowStepTest, HoldsPhaseInvocation)
{
    const beez::core::WorkflowStep Step {
        .invocation = beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}};

    EXPECT_EQ(Step.invocation.phase, "compile");
    EXPECT_EQ(Step.invocation.scope, "code");
}
