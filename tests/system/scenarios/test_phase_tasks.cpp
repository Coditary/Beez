#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

TEST(SystemPhaseTaskTest, GenerateDocsTaskRunsByName)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"gen-docs"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("docs.out"));
}

TEST(SystemPhaseTaskTest, GenerateCodeTaskRunsByName)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"gen-code"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("code.out"));
}

TEST(SystemPhaseTaskTest, CompileTaskRunsByName)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"compile"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("build.out"));
}

TEST(SystemPhaseTaskTest, UnknownPhaseTaskFails)
{
    const beez::test::FixtureProject Project("phase-tasks");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"link"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "name not found in registry"));
}
