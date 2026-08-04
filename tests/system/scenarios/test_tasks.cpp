#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(SystemTaskTest, HelloTaskCreatesOutputFile)
{
    const beez::test::FixtureProject Project("minimal");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("hello.out"));
}

TEST(SystemTaskTest, FailTaskReturnsNonZeroExitCode)
{
    const beez::test::FixtureProject Project("minimal");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"fail"});
    EXPECT_NE(Result.exitCode, 0);
}

TEST(SystemTaskTest, MultipleTaskInvocationsInSequence)
{
    const beez::test::FixtureProject Project("minimal");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"hello"}).exitCode, 0);
    ASSERT_TRUE(Project.hasFile("hello.out"));

    const beez::test::ProcessResult CleanResult = beez::test::runBeez(Project.path(), {"clean"});
    EXPECT_EQ(CleanResult.exitCode, 0);
    EXPECT_FALSE(Project.hasFile("hello.out"));
}

TEST(SystemTaskTest, UnknownOrphanTaskFails)
{
    const beez::test::FixtureProject Project("minimal");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"does-not-exist"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "name not found in registry"));
}
