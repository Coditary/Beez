#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

TEST(SystemErrorTest, MissingArgumentsShowsUsage)
{
    const beez::test::FixtureProject Project("minimal");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "Usage: beez"));
}

TEST(SystemErrorTest, MissingBuildScriptFails)
{
    const beez::test::FixtureProject Project("empty");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "build script not found"));
}

TEST(SystemErrorTest, InvalidBuildScriptFails)
{
    const beez::test::FixtureProject Project("invalid-syntax");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "failed to load build script"));
}

TEST(SystemErrorTest, UnknownTaskInValidProjectFails)
{
    const beez::test::FixtureProject Project("minimal");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"typo-task"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "name not found in registry"));
}

TEST(SystemErrorTest, TaskNameShadowedByNothingStillFailsForGarbageInput)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {""});
    EXPECT_NE(Result.exitCode, 0);
}
