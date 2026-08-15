#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(SystemListEntitiesTest, ListWorkflowsPrintsFixtureWorkflows)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "workflows"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "workflows:"));
    EXPECT_TRUE(beez::test::outputContains(Result, "build"));
    EXPECT_TRUE(beez::test::outputContains(Result, "ci"));
}

TEST(SystemListEntitiesTest, ListStepsPrintsFixtureSteps)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "steps"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "steps:"));
    EXPECT_TRUE(beez::test::outputContains(Result, "gen-code"));
    EXPECT_TRUE(beez::test::outputContains(Result, "compile"));
    EXPECT_TRUE(beez::test::outputContains(Result, "gen-docs"));
}

TEST(SystemListEntitiesTest, ListPhasesPrintsFixturePhases)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "phases"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "phases:"));
    EXPECT_TRUE(beez::test::outputContains(Result, "generate"));
    EXPECT_TRUE(beez::test::outputContains(Result, "compile"));
}

TEST(SystemListEntitiesTest, ListTasksIsEmptyForWorkflowOnlyFixture)
{
    const beez::test::FixtureProject Project("workflows");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "tasks"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "tasks:"));
    EXPECT_FALSE(beez::test::outputContainsTaskName(Result, "build"));
}

TEST(SystemListEntitiesTest, ListTasksPrintsFlagMatrixTasks)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "tasks"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "tasks:"));
    EXPECT_TRUE(beez::test::outputContains(Result, "hello"));
    EXPECT_TRUE(beez::test::outputContains(Result, "noop"));
}
