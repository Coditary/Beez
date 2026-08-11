#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(CliInitTest, InitHelpShowsTempifyBanner)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--init", "--help"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Tempify - Local Template Generator"), std::string::npos);
    EXPECT_NE(Result.output.find("tempify <command> [args...] [options]"), std::string::npos);
}

TEST(CliInitTest, InitWithoutArgsShowsTempifyHelp)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--init"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Tempify - Local Template Generator"), std::string::npos);
}

TEST(CliInitTest, InitVersionShowsTempifyVersion)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--init", "--version"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("0.1.2"), std::string::npos);
}

TEST(CliInitTest, InitPrebyteFlagDoesNotTriggerBeezPhaseParsing)
{
    const beez::test::TempProject Project;
    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--init", "-p", "--help"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Embedded Prebyte Passthrough"), std::string::npos);
}

TEST(CliInitTest, BeezListStillWorksWithoutInit)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("alpha", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--list", "tasks"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("tasks:"), std::string::npos);
    EXPECT_NE(Result.output.find("alpha"), std::string::npos);
}

TEST(CliInitTest, BeezPhaseFlagStillWorksWithoutInit)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "echo gen > gen.out",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-p", "generate:code", "--dry-run"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Starting Phase: generate"), std::string::npos);
}
