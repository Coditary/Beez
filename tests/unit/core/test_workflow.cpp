#include "beez/core/phase_invocation.hpp"
#include "beez/core/workflow_step.hpp"

#include <gtest/gtest.h>

TEST(WorkflowStepTest, SingleInvocationIsSequential)
{
    const beez::core::WorkflowStep Step {
        .invocations = {beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}}};

    EXPECT_FALSE(Step.isParallel());
}

TEST(WorkflowStepTest, MultipleInvocationsAreParallel)
{
    const beez::core::WorkflowStep Step {
        .invocations = {beez::core::PhaseInvocation {.phase = "generate", .scope = "docs"},
                        beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}}};

    EXPECT_TRUE(Step.isParallel());
}
