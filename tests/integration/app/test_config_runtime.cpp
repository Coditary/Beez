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

}  // namespace

TEST(ConfigRuntimeIntegrationTest, BuildLuaConfigUsesCustomCachePath)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
beez.config({
    cache = {
        path = "project-cache",
    },
})
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/main.o" },
    run = "mkdir -p build && echo object > build/main.o",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-s", "compile", "--no-log-file"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "project-cache" / "entries"));
    EXPECT_FALSE(std::filesystem::exists(Project.path() / ".cache"));
}

TEST(ConfigRuntimeIntegrationTest, ShowConfigReflectsBuildLuaOverrides)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    cache = {
        path = "runtime-cache",
        protect = true,
    },
    performance = {
        max_threads = 5,
    },
})
task("noop", "true")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("runtime-cache"), std::string::npos);
    EXPECT_NE(Result.output.find("cache.protect"), std::string::npos);
    EXPECT_NE(Result.output.find("performance.max_threads"), std::string::npos);
}

TEST(ConfigRuntimeIntegrationTest, CliNoCacheOverridesProjectCacheEnabled)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");
    Project.writeBuildLua(R"(
beez.config({
    cache = {
        enabled = true,
    },
})
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

    const auto RunsPath = Project.path() / "build" / "runs.txt";
    std::ifstream beforeStream(RunsPath);
    std::size_t linesBefore = 0;
    std::string line;
    while (std::getline(beforeStream, line))
    {
        ++linesBefore;
    }
    EXPECT_EQ(linesBefore, 1U);

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-s", "compile", "--no-cache"});
    EXPECT_EQ(Result.exitCode, 0);

    std::ifstream afterStream(RunsPath);
    std::size_t linesAfter = 0;
    while (std::getline(afterStream, line))
    {
        ++linesAfter;
    }
    EXPECT_EQ(linesAfter, 2U);
}

TEST(ConfigRuntimeIntegrationTest, EnvVarsFromBuildLuaAreAppliedToProcess)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    env = {
        vars = {
            BEEZ_INTEGRATION_ENV_TEST = "from-build-lua",
        },
    },
})
task("check-env", "test \"$BEEZ_INTEGRATION_ENV_TEST\" = from-build-lua")
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"check-env"});
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(ConfigRuntimeIntegrationTest, CliVerboseOverridesProjectOutputMode)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    ui = {
        output_mode = "silent",
    },
})
task("echo-task", "echo beez-config-verbose-output")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo-task", "--verbose"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("beez-config-verbose-output"), std::string::npos);
}
