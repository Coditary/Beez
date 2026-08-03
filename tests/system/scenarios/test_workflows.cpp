#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

TEST(SystemWorkflowTest, BuildWorkflowIsNotExecutableYet)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "workflow execution is not yet implemented"));
}

TEST(SystemWorkflowTest, CiWorkflowIsNotExecutableYet)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"ci"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "workflow execution is not yet implemented"));
}

TEST(SystemWorkflowTest, PhaseBoundTasksRemainCallableByName)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult GenResult = beez::test::runBeez(Project.path(), {"gen-code"});
    EXPECT_EQ(GenResult.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("gen.out"));

    const beez::test::ProcessResult CompileResult =
        beez::test::runBeez(Project.path(), {"compile"});
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
