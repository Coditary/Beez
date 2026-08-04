#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

TEST(SystemWorkflowTest, BuildWorkflowRunsPhaseStepsInOrder)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("gen.out"));
    EXPECT_TRUE(Project.hasFile("compile.out"));
}

TEST(SystemWorkflowTest, CiWorkflowRunsParallelAndSequentialPhases)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"ci"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("wf-docs.out"));
    EXPECT_TRUE(Project.hasFile("gen.out"));
    EXPECT_TRUE(Project.hasFile("compile.out"));
}

TEST(SystemWorkflowTest, StepsAreCallableByNameWithFlag)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult GenResult =
        beez::test::runBeez(Project.path(), {"-s", "gen-code"});
    EXPECT_EQ(GenResult.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("gen.out"));

    const beez::test::ProcessResult CompileResult =
        beez::test::runBeez(Project.path(), {"-s", "compile"});
    EXPECT_EQ(CompileResult.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("compile.out"));
}

TEST(SystemWorkflowTest, UnknownWorkflowNameFails)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"release"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "name not found in registry"));
}

TEST(SystemWorkflowTest, PhaseInvocationRunsAllScopesWhenUnscoped)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-p", "generate"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("wf-docs.out"));
    EXPECT_TRUE(Project.hasFile("gen.out"));
}
