#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{

void writeProjectFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

std::size_t countLines(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    std::size_t count = 0;
    std::string line;
    while (std::getline(stream, line))
    {
        ++count;
    }
    return count;
}

}  // namespace

TEST(CliFlagsIntegrationTest, SilentModeSuppressesSuccessOutput)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("echo-task", "echo beez-silent-hidden")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo-task", "--silent"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_EQ(Result.output.find("beez-silent-hidden"), std::string::npos);
    EXPECT_EQ(Result.output.find("Build finished"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, SilentModeSuppressesRegistryErrors)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"known\", \"true\")\n");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"missing", "--silent"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_EQ(Result.output.find("name not found in registry"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ErrorModeSuppressesSuccessSummary)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("echo-task", "echo beez-error-hidden")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo-task", "--error"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_EQ(Result.output.find("Build finished"), std::string::npos);
    EXPECT_EQ(Result.output.find("beez-error-hidden"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ErrorModeShowsFailureOutput)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("fail-task", "echo beez-error-failure >&2; exit 1")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"fail-task", "--error"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("beez-error-failure"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ShowConfigPrintsMergedConfiguration)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    performance = {
        max_threads = 3,
    },
})
task("noop", "true")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("=== Beez Active Configuration ==="), std::string::npos);
    EXPECT_NE(Result.output.find("performance.max_threads"), std::string::npos);
    EXPECT_NE(Result.output.find("build.lua"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ConfigOptionsPrintsSchemaForPath)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--config-options", "cache.hash.algorithm"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("=== cache.hash.algorithm ==="), std::string::npos);
    EXPECT_NE(Result.output.find("Kind: enum"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ConfigOptionsRejectsUnknownPath)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--config-options", "not.a.real.path"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("unknown config option path"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, DumpCompletionPrintsBashScript)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--dump-completion", "bash"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("complete"), std::string::npos);
    EXPECT_NE(Result.output.find("--config-options"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ListWorkflowsPrintsRegisteredNames)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("build", {
    { phase = "generate", scope = "code" },
})
task("noop", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "workflows"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("workflows:"), std::string::npos);
    EXPECT_NE(Result.output.find("build"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ListStepsPrintsStepTable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    description = "Generate sources",
    run = "true",
})
task("noop", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "steps"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("steps:"), std::string::npos);
    EXPECT_NE(Result.output.find("gen-code"), std::string::npos);
    EXPECT_NE(Result.output.find("generate"), std::string::npos);
    EXPECT_NE(Result.output.find("Generate sources"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, ListPhasesPrintsPhaseScopes)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "true",
})
task("noop", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "phases"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("phases:"), std::string::npos);
    EXPECT_NE(Result.output.find("generate"), std::string::npos);
    EXPECT_NE(Result.output.find("code"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, UpdateFlagReportsCacheMaintenance)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--update"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Updated Beez cache"), std::string::npos);
}

TEST(CliFlagsIntegrationTest, LogFileFlagWritesRunLogToCustomPath)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");
    const auto LogPath = Project.path() / "custom-run.log";

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"noop", "--log-file", LogPath.string()});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(LogPath));
}

TEST(CliFlagsIntegrationTest, NoLogFileFlagSkipsDefaultRunLog)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");
    const auto DefaultLogPath = Project.path() / ".cache" / "logs" / "latest.log";

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"noop", "--no-log-file"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(std::filesystem::exists(DefaultLogPath));
}

TEST(CliFlagsIntegrationTest, NoCacheFlagForcesStepReexecution)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/runs.txt" },
    run = "mkdir -p build && echo run >> build/runs.txt",
})
)");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"-s", "compile"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"-s", "compile"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    const beez::test::ProcessResult NoCacheResult =
        beez::test::runBeez(Project.path(), {"-s", "compile", "--no-cache"});
    EXPECT_EQ(NoCacheResult.exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(CliFlagsIntegrationTest, ThreadsFlagAcceptsJobsAlias)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"noop", "-j", "2"});
    EXPECT_EQ(Result.exitCode, 0);
}
