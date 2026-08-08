#include "beez/core/cache_options.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/success_cache.hpp"

#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

beez::core::StepIdentity makeLintIdentity()
{
    return beez::core::StepIdentity {.name = "lint", .phase = "lint", .scope = "cpp"};
}

class SuccessCacheTest : public ::testing::Test
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  protected:
    void SetUp() override
    {
        root = std::filesystem::temp_directory_path() /
               ("beez_success_cache_" + std::to_string(counter++));
        std::filesystem::create_directories(root);
        cacheDir = root / ".cache";
        matcher = beez::core::makeSimpleGlobMatcher();
        beez::core::CacheOptions cacheOptions;
        cacheOptions.root = cacheDir;
        cache = std::make_unique<beez::core::SuccessCache>(cacheOptions, *matcher);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(root);
    }

    beez::core::SuccessCacheSession openSession(const beez::core::StepConfigPtr& config = nullptr)
    {
        return cache->openSession(makeLintIdentity(), root, config);
    }

    std::filesystem::path root;
    std::filesystem::path cacheDir;
    std::unique_ptr<beez::core::IGlobMatcher> matcher;
    std::unique_ptr<beez::core::SuccessCache> cache;
    static inline int counter = 0;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

}  // namespace

TEST_F(SuccessCacheTest, MissWhenNoSuccessEntryExists)
{
    auto session = openSession();
    EXPECT_FALSE(session.successCached("target-a"));
    EXPECT_FALSE(session.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, HitAfterCacheSuccess)
{
    auto session = openSession();
    session.cacheSuccess("target-a");
    session.finish();

    auto nextSession = openSession();
    EXPECT_TRUE(nextSession.successCached("target-a"));
}

TEST_F(SuccessCacheTest, HitAfterCacheFileSuccess)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    auto session = openSession();
    session.cacheFileSuccess("src/main.cpp");
    session.finish();

    auto nextSession = openSession();
    EXPECT_TRUE(nextSession.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, LookupReturnsStoredFileDurationSeconds)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    auto session = openSession();
    constexpr double KDuration = 0.42;
    session.cacheFileSuccess("src/main.cpp", KDuration);
    session.finish();

    auto nextSession = openSession();
    ASSERT_TRUE(nextSession.fileSuccessCached("src/main.cpp"));
    EXPECT_DOUBLE_EQ(nextSession.fileSavedDurationSeconds("src/main.cpp"), KDuration);
}

TEST_F(SuccessCacheTest, MissWhenFileContentChanges)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    auto session = openSession();
    session.cacheFileSuccess("src/main.cpp");
    session.finish();

    writeFile(root / "src" / "main.cpp", "int main() { return 1; }\n");

    auto nextSession = openSession();
    EXPECT_FALSE(nextSession.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, MissWhenIncludedHeaderChanges)
{
    writeFile(root / "include" / "widget.hpp", "#pragma once\nstruct Widget {};\n");
    writeFile(root / "src" / "main.cpp", R"(
#include "include/widget.hpp"
int main() {}
)");

    auto session = openSession();
    session.cacheFileSuccess("src/main.cpp");
    session.finish();

    writeFile(root / "include" / "widget.hpp", "#pragma once\nstruct Widget { int id; };\n");

    auto nextSession = openSession();
    EXPECT_FALSE(nextSession.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, FailureIsNotCached)
{
    auto session = openSession();
    session.recordFileCacheMiss("src/main.cpp");
    session.finish();

    auto nextSession = openSession();
    EXPECT_FALSE(nextSession.fileSuccessCached("src/main.cpp"));
    EXPECT_EQ(nextSession.getCacheMisses().size(), 1U);
    EXPECT_EQ(nextSession.getCacheMisses().front(), "src/main.cpp");
}

TEST_F(SuccessCacheTest, SuccessClearsPreviousMiss)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    auto firstSession = openSession();
    firstSession.recordFileCacheMiss("src/main.cpp");
    firstSession.finish();

    auto secondSession = openSession();
    secondSession.cacheFileSuccess("src/main.cpp");
    secondSession.finish();

    auto thirdSession = openSession();
    EXPECT_TRUE(thirdSession.fileSuccessCached("src/main.cpp"));
    EXPECT_TRUE(thirdSession.getCacheMisses().empty());
}

TEST_F(SuccessCacheTest, HitAfterCacheFileSuccessWithMultilineConfigFingerprint)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    const auto Config = beez::test::makeTestConfig("compdb=build\nlint_rev=2\npatterns=[a,b]");
    auto session = openSession(Config);
    session.cacheFileSuccess("src/main.cpp");
    session.finish();

    auto nextSession = openSession(Config);
    EXPECT_TRUE(nextSession.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, MissWhenConfigFingerprintChanges)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    const auto ConfigA = beez::test::makeTestConfig("flags=-Wall");
    auto session = openSession(ConfigA);
    session.cacheFileSuccess("src/main.cpp");
    session.finish();

    const auto ConfigB = beez::test::makeTestConfig("flags=-Wextra");
    auto nextSession = openSession(ConfigB);
    EXPECT_FALSE(nextSession.fileSuccessCached("src/main.cpp"));
}

TEST_F(SuccessCacheTest, GetCacheMissesReturnsPreviousRunMisses)
{
    auto firstSession = openSession();
    firstSession.recordCacheMiss("target-a");
    firstSession.recordCacheMiss("target-b");
    firstSession.finish();

    auto secondSession = openSession();
    const auto& misses = secondSession.getCacheMisses();
    ASSERT_EQ(misses.size(), 2U);
    EXPECT_EQ(misses.at(0), "target-a");
    EXPECT_EQ(misses.at(1), "target-b");
}
