#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

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

std::vector<std::filesystem::path> listManifests(const std::filesystem::path& entriesDir)
{
    std::vector<std::filesystem::path> manifests;
    if (!std::filesystem::is_directory(entriesDir))
    {
        return manifests;
    }

    for (const auto& entry : std::filesystem::directory_iterator(entriesDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".manifest")
        {
            manifests.push_back(entry.path());
        }
    }

    return manifests;
}

void overwriteManifests(const std::filesystem::path& entriesDir, const std::string& payload)
{
    for (const auto& manifest : listManifests(entriesDir))
    {
        std::ofstream stream(manifest, std::ios::binary | std::ios::trunc);
        stream.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    }
}

void expectNoCrash(const beez::test::ProcessResult& result, const std::string& context)
{
    EXPECT_FALSE(result.terminatedBySignal) << context << "\n" << result.output;
    EXPECT_TRUE(beez::test::exitedNormally(result)) << context << "\n" << result.output;
}

}  // namespace

TEST(SystemCacheAdversarialTest, CorruptManifestFallsBackToReexecution)
{
    const beez::test::FixtureProject Project("cache-adversarial");
    const auto EntriesDir = Project.path() / "adversarial-cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);
    ASSERT_FALSE(listManifests(EntriesDir).empty());

    overwriteManifests(EntriesDir, std::string(32U, '\xff'));

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    expectNoCrash(Result, "corrupt manifest rebuild");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(SystemCacheAdversarialTest, LegacyCacheEnvelopeFailsGracefully)
{
    const beez::test::FixtureProject Project("cache-adversarial");
    const auto EntriesDir = Project.path() / "adversarial-cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_FALSE(listManifests(EntriesDir).empty());

    const std::string LegacyEnvelope =
        "BEEZCACHE1\nalgorithm=gzip\nlevel=6\nmode=always\n---\nnot-valid\n";
    overwriteManifests(EntriesDir, LegacyEnvelope);

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    expectNoCrash(Result, "legacy cache envelope");
    EXPECT_NE(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "legacy cache envelope") ||
                beez::test::outputContains(Result, "Fatal error"))
        << Result.output;
}

TEST(SystemCacheAdversarialTest, TruncatedManifestDoesNotCrash)
{
    const beez::test::FixtureProject Project("cache-behavior");
    const auto EntriesDir = Project.path() / ".cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_FALSE(listManifests(EntriesDir).empty());

    overwriteManifests(EntriesDir, "algorithm=gzip\nlev");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    expectNoCrash(Result, "truncated manifest");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
}

TEST(SystemCacheAdversarialTest, UnexpectedFilesInEntriesDoNotCrash)
{
    const beez::test::FixtureProject Project("cache-adversarial");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);

    Project.writeFile("adversarial-cache/entries/plain-text.txt", "not a manifest");
    Project.writeFile("adversarial-cache/entries/nested/garbage.bin", std::string(8U, '\0'));
    Project.writeFile("adversarial-cache/not-a-directory", "blocker");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    expectNoCrash(Result, "unexpected cache files");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
}

TEST(SystemCacheAdversarialTest, UpdateOnCorruptCacheSucceeds)
{
    const beez::test::FixtureProject Project("config-cache");
    const auto EntriesDir = Project.path() / "fixture-cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_FALSE(listManifests(EntriesDir).empty());

    overwriteManifests(EntriesDir, std::string(24U, '\xff'));

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--update"});
    expectNoCrash(Result, "update on corrupt cache");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(beez::test::outputContains(Result, "Updated Beez cache")) << Result.output;
}

TEST(SystemCacheAdversarialTest, CleanCacheAfterCorruptionAllowsFreshRun)
{
    const beez::test::FixtureProject Project("cache-behavior");
    const auto EntriesDir = Project.path() / ".cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    overwriteManifests(EntriesDir, std::string(64U, '\x00'));
    Project.writeFile(".cache/entries/rogue.manifest", "rogue");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--clean-cache"}).exitCode, 0);

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"build"});
    expectNoCrash(Result, "clean cache after corruption");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}

TEST(SystemCacheAdversarialTest, NoCacheBypassesCorruptManifest)
{
    const beez::test::FixtureProject Project("cache-adversarial");
    const auto EntriesDir = Project.path() / "adversarial-cache" / "entries";

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"build"}).exitCode, 0);
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 1U);

    overwriteManifests(EntriesDir, "broken-cache");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"build", "--no-cache"});
    expectNoCrash(Result, "no-cache with corrupt manifest");
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_EQ(countLines(Project.path() / "build" / "runs.txt"), 2U);
}
