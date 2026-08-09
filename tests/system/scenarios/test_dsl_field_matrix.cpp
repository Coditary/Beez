#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace
{

std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(SystemDslFieldMatrixTest, MinimalValidFixtureRunsEndToEnd)
{
    const beez::test::FixtureProject Project("dsl-minimal-valid");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"run"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    ASSERT_TRUE(Project.hasFile("out.txt"));
    EXPECT_EQ(readTextFile(Project.path() / "out.txt"), "minimal-valid\n");
}

TEST(SystemDslFieldMatrixTest, TaskReferencingMissingStepFailsAtLoad)
{
    const beez::test::FixtureProject Project("dsl-task-missing-step");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"run"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_NE(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "failed to load build script")) << Result.output;
}

TEST(SystemDslFieldMatrixTest, WorkflowWithNoRegisteredStepsFailsToLoad)
{
    const beez::test::FixtureProject Project("dsl-workflow-no-steps");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"run"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_NE(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "failed to load build script")) << Result.output;
}

TEST(SystemDslFieldMatrixTest, PartialStepFixtureFailsToLoad)
{
    const beez::test::FixtureProject Project("dsl-partial-step");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"ci"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_NE(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "failed to load build script")) << Result.output;
}

TEST(SystemDslFieldMatrixTest, UnknownConfigKeysWarnButStillRun)
{
    const beez::test::FixtureProject Project("dsl-config-unknown-key");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_FALSE(Result.terminatedBySignal) << Result.output;
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "unknown beez.config key")) << Result.output;
    ASSERT_TRUE(Project.hasFile("out.txt"));
}
