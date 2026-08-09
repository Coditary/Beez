#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

TEST(SystemCliFlagsTest, SilentModeSuppressesSuccessOutput)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo", "--silent"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(beez::test::outputContains(Result, "flag-matrix-output"));
    EXPECT_FALSE(beez::test::outputContains(Result, "Build finished"));
}

TEST(SystemCliFlagsTest, ErrorModeShowsFailureOutput)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"fail", "--error"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "task execution failed"));
}

TEST(SystemCliFlagsTest, DryRunSkipsTaskSideEffects)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"hello", "--dry-run"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(Project.hasFile("hello.out"));
}

TEST(SystemCliFlagsTest, VerboseModeShowsCommandOutput)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"echo", "--verbose"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "flag-matrix-output"));
}

TEST(SystemCliFlagsTest, CleanCacheRemovesCacheDirectory)
{
    const beez::test::FixtureProject Project("flag-matrix");
    const auto CachePath = Project.path() / ".cache";
    std::filesystem::create_directories(CachePath / "entries");
    {
        std::ofstream stream(CachePath / "entries" / "marker.txt");
        stream << "cached\n";
    }

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--clean-cache"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(std::filesystem::exists(CachePath));
    EXPECT_TRUE(beez::test::outputContains(Result, "Removed Beez cache"));
}

TEST(SystemCliFlagsTest, UpdateFlagReportsCacheMaintenance)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--update"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "Updated Beez cache"));
}

TEST(SystemCliFlagsTest, NoLogFileSkipsDefaultRunLog)
{
    const beez::test::FixtureProject Project("flag-matrix");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"noop", "--no-log-file"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_FALSE(Project.hasFile(".cache/logs/latest.log"));
}

TEST(SystemCliFlagsTest, ShowConfigPrintsActiveConfiguration)
{
    const beez::test::FixtureProject Project("config-cache");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "=== Beez Active Configuration ==="));
    EXPECT_TRUE(beez::test::outputContains(Result, "fixture-cache"));
}
