#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{

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

TEST(SystemCacheUsecaseTest, SecondRunSkipsCachedArtifactStep)
{
    const beez::test::FixtureProject Project("cache-behavior");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);
}

TEST(SystemCacheUsecaseTest, NoCacheFlagForcesStepReexecution)
{
    const beez::test::FixtureProject Project("cache-behavior");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"build", "--no-cache"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(SystemCacheUsecaseTest, CleanCacheForcesReexecutionOnNextRun)
{
    const beez::test::FixtureProject Project("cache-behavior");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--clean-cache"}).exitCode, 0);

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(SystemCacheUsecaseTest, StepInvocationUsesCacheAcrossRuns)
{
    const beez::test::FixtureProject Project("cache-behavior");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"-s", "compile"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"-s", "compile"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);
}
