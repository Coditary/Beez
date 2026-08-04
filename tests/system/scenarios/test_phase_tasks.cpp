#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

TEST(SystemPhaseTaskTest, GenerateDocsStepRunsWithFlag)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-s", "gen-docs"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("docs.out"));
}

TEST(SystemPhaseTaskTest, GenerateCodeStepRunsWithFlag)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-s", "gen-code"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("code.out"));
}

TEST(SystemPhaseTaskTest, CompileStepRunsWithFlag)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-s", "compile"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("build.out"));
}

TEST(SystemPhaseTaskTest, PhaseInvocationRunsMatchingSteps)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-p", R"(generate["docs"])"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("docs.out"));
}

TEST(SystemPhaseTaskTest, UnknownStepFails)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-s", "link"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "name not found in registry"));
}
