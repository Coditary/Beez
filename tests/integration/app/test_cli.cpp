#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

TEST(CliTest, MissingArgumentsShowsUsage)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Beez - Build Everything Easy"), std::string::npos);
    EXPECT_NE(Result.output.find("Usage: beez [target] [core-options] [-- user-options]"),
              std::string::npos);
}

TEST(CliTest, HelpFlagShowsBanner)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--help"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Beez - Build Everything Easy (0.1.0)"), std::string::npos);
    EXPECT_NE(Result.output.find("-h, --help"), std::string::npos);
    EXPECT_NE(Result.output.find("--verbose"), std::string::npos);
}

TEST(CliTest, VersionFlagShowsBeezAndLuaVersions)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--version"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Beez 0.1.0"), std::string::npos);
    EXPECT_NE(Result.output.find("Lua 5.4"), std::string::npos);
}

TEST(CliTest, ListTasksPrintsRegisteredNames)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("alpha", "true")
task("beta", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "tasks"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("tasks:"), std::string::npos);
    EXPECT_NE(Result.output.find("alpha"), std::string::npos);
    EXPECT_NE(Result.output.find("beta"), std::string::npos);
}

TEST(CliTest, DryRunDoesNotExecuteShellCommands)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("mark", "touch .dry-run-should-not-exist")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"mark", "--dry-run"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(std::filesystem::exists(Project.path() / ".dry-run-should-not-exist"));
    EXPECT_NE(Result.output.find("Starting Task: mark"), std::string::npos);
}

TEST(CliTest, VerboseModeIncludesCommandOutput)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("echo-task", "echo beez-verbose-output")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo-task", "--verbose"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("beez-verbose-output"), std::string::npos);
}

TEST(CliTest, ReportsElapsedDurationForSlowTask)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("slow", "sleep 0.2")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"slow"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_EQ(Result.output.find("Build finished in 0.00s"), std::string::npos);
}

TEST(CliTest, CleanModeSuppressesCommandOutput)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("echo-task", "echo beez-clean-hidden-output")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"echo-task"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("| echo beez-clean-hidden-output"), std::string::npos);
    EXPECT_EQ(Result.output.find("  | beez-clean-hidden-output"), std::string::npos);
}

TEST(CliTest, CleanModeShowsCommandOutputOnFailure)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("fail-task", "echo beez-clean-failure-output >&2; exit 1")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"fail-task"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("beez-clean-failure-output"), std::string::npos);
}

TEST(CliTest, CleanModeShowsOnlyFailedWorkerOutput)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    ui = {
        prefix = true,
        prefix_format = "Worker {id}",
    },
})
step({
    name = "qa-fail",
    phase = "qa",
    scope = "cli-fail",
    run = function(ctx)
        print("[status] skip (cached): noise-a")
        print("[status] skip (cached): noise-b")
        local job = ctx:spawn({
            name = "bad",
            cmd = "echo worker-failure-output >&2; exit 1",
        })
        return ctx:wait(job)
    end,
})
task("fail-worker", { { name = "qa-fail" } })
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"fail-worker"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("worker-failure-output"), std::string::npos);
    EXPECT_NE(Result.output.find("Worker bad"), std::string::npos);
    EXPECT_EQ(Result.output.find("Worker 0"), std::string::npos);
    EXPECT_EQ(Result.output.find("noise-a"), std::string::npos);
    EXPECT_EQ(Result.output.find("noise-b"), std::string::npos);
}

TEST(CliTest, CleanModeShowsCommandInProgressLine)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("build", {
    "echo beez-progress-command",
})
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("| echo beez-progress-command"), std::string::npos);
    EXPECT_EQ(Result.output.find("task: build"), std::string::npos);
}

TEST(CliTest, CleanModeShowsStepDescriptionInProgressLine)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    description = "Generate C++ headers",
    run = "true",
})
task("build", { { name = "gen-code" } })
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("| Generate C++ headers"), std::string::npos);
    EXPECT_EQ(Result.output.find("step: gen-code"), std::string::npos);
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
    EXPECT_EQ(Result.output.find("Did you mean"), std::string::npos);
}

TEST(CliTest, MisspelledTaskShowsDidYouMean)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"build\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"biuld"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("name not found in registry"), std::string::npos);
    EXPECT_NE(Result.output.find("Did you mean 'build'?"), std::string::npos);
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

TEST(CliTest, CleanCacheRemovesCacheDirectory)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");
    const auto CachePath = Project.path() / ".cache";
    std::filesystem::create_directories(CachePath / "entries");
    {
        std::ofstream stream(CachePath / "entries" / "marker.txt");
        stream << "cached\n";
    }

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--clean-cache"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(std::filesystem::exists(CachePath));
    EXPECT_NE(Result.output.find("Removed Beez cache"), std::string::npos);
}
