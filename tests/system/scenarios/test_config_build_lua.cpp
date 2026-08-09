#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

TEST(SystemConfigBuildLuaTest, CustomCachePathStoresEntriesOutsideDefaultCache)
{
    const beez::test::FixtureProject Project("config-cache");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"build", "--no-log-file"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("build/main.o"));
    EXPECT_TRUE(Project.hasFile("fixture-cache/entries"));
    EXPECT_FALSE(Project.hasFile(".cache"));
}

TEST(SystemConfigBuildLuaTest, ShowConfigReflectsProjectOverrides)
{
    const beez::test::FixtureProject Project("config-cache");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "fixture-cache"));
    EXPECT_TRUE(beez::test::outputContains(Result, "cache.protect"));
    EXPECT_TRUE(beez::test::outputContains(Result, "performance.max_threads"));
    EXPECT_TRUE(beez::test::outputContains(Result, "build.lua"));
}

TEST(SystemConfigBuildLuaTest, UiOutputModeFromConfigAffectsRuntimeOutput)
{
    const beez::test::FixtureProject Project("config-ui");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"echo"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "ui-fixture-output"));
}

TEST(SystemConfigBuildLuaTest, EnvVarsFromConfigAreVisibleToTasks)
{
    const beez::test::FixtureProject Project("config-env");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"check"});
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(SystemConfigBuildLuaTest, CliNoCacheOverridesProjectCacheSetting)
{
    const beez::test::FixtureProject Project("cache-behavior");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);

    std::ifstream beforeStream(Project.path() / "build" / "runs.txt");
    std::size_t linesBefore = 0;
    std::string line;
    while (std::getline(beforeStream, line))
    {
        ++linesBefore;
    }
    EXPECT_EQ(linesBefore, 1U);

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"build", "--no-cache"});
    EXPECT_EQ(Result.exitCode, 0);

    std::ifstream afterStream(Project.path() / "build" / "runs.txt");
    std::size_t linesAfter = 0;
    while (std::getline(afterStream, line))
    {
        ++linesAfter;
    }
    EXPECT_EQ(linesAfter, 2U);
}
