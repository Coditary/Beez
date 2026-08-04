#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(CliTest, MissingArgumentsShowsUsage)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Usage: beez"), std::string::npos);
}

TEST(CliTest, MissingBuildScriptExitsWithError)
{
    const beez::test::TempProject Project;

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"noop"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("build.lua not found"), std::string::npos);
}

TEST(CliTest, RunsOrphanTaskSuccessfully)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("mark", "touch .cli-ran")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"mark"});
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(CliTest, UnknownTaskExitsWithError)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"known\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"missing"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("name not found in registry"), std::string::npos);
}

TEST(CliTest, WorkflowInvocationRunsMatchingSteps)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "touch .workflow-ran",
})
workflow("build", {
    { phase = "generate", scope = "code" },
})
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".workflow-ran"));
}

TEST(CliTest, PhaseInvocationRunsMatchingStepsWithColonSyntax)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "touch .phase-colon-ran",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-p", "generate:code"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".phase-colon-ran"));
}

TEST(CliTest, PhaseInvocationRunsMatchingSteps)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "touch .phase-ran",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-p", R"(generate["code"])"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".phase-ran"));
}

TEST(CliTest, StepInvocationRunsSingleStep)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = "touch .step-ran",
})
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-s", "compile"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".step-ran"));
}
