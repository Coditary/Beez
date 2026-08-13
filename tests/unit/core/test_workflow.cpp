#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_resolution.hpp"
#include "beez/core/model/workflow_stage.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <gtest/gtest.h>

TEST(WorkflowStepTest, HoldsPhaseInvocation)
{
    const beez::core::WorkflowStep Step {
        .invocation = beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}};

    EXPECT_EQ(Step.invocation.phase, "compile");
    EXPECT_EQ(Step.invocation.scope, "code");
}

TEST(WorkflowResolutionTest, ResolvesCumulativeStagesThroughTarget)
{
    beez::core::Workflow workflow;
    workflow.name = "release";
    workflow.stages = {
        beez::core::WorkflowStage {
            .name = "prepare",
            .invocations =
                {
                    beez::core::PhaseInvocation {.phase = "clean", .scope = "artifacts"},
                    beez::core::PhaseInvocation {.phase = "deps", .scope = "install"},
                },
        },
        beez::core::WorkflowStage {
            .name = "compile",
            .invocations =
                {
                    beez::core::PhaseInvocation {.phase = "build", .scope = "backend"},
                },
        },
        beez::core::WorkflowStage {
            .name = "verify",
            .invocations =
                {
                    beez::core::PhaseInvocation {.phase = "test", .scope = "unit"},
                },
        },
    };

    const auto PrepareSteps = beez::core::resolveWorkflowExecutionSteps(workflow, "prepare");
    ASSERT_EQ(PrepareSteps.size(), 2U);
    EXPECT_EQ(PrepareSteps[0].invocation.phase, "clean");
    EXPECT_EQ(PrepareSteps[1].invocation.phase, "deps");

    const auto VerifySteps = beez::core::resolveWorkflowExecutionSteps(workflow, "verify");
    ASSERT_EQ(VerifySteps.size(), 4U);
    EXPECT_EQ(VerifySteps[2].invocation.phase, "build");
    EXPECT_EQ(VerifySteps[3].invocation.phase, "test");
}

TEST(WorkflowResolutionTest, RejectsUnknownStageTarget)
{
    beez::core::Workflow workflow;
    workflow.name = "release";
    workflow.stages = {beez::core::WorkflowStage {
        .name = "prepare",
        .invocations = {beez::core::PhaseInvocation {.phase = "clean", .scope = "artifacts"}},
    }};

    EXPECT_THROW(beez::core::resolveWorkflowExecutionSteps(workflow, "missing"),
                 std::runtime_error);
}
