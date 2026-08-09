#include "beez/core/cache/step_cache.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"

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

beez::core::Step makeCompileStep()
{
    beez::core::Step step;
    step.name = "compile";
    step.phase = "compile";
    step.scope = "cpp";
    step.shellRun = "echo compile";
    step.input = {"src/**/*.cpp"};
    step.output = {"build/**/*.o"};
    return step;
}

class StepCacheTest : public ::testing::Test
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  protected:
    void SetUp() override
    {
        root = std::filesystem::temp_directory_path() /
               ("beez_step_cache_" + std::to_string(counter++));
        std::filesystem::create_directories(root);
        cacheDir = root / ".cache";
        matcher = beez::core::makeSimpleGlobMatcher();
        beez::core::CacheOptions cacheOptions;
        cacheOptions.root = cacheDir;
        cache = std::make_unique<beez::core::StepCache>(cacheOptions, *matcher);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(root);
    }

    std::filesystem::path root;
    std::filesystem::path cacheDir;
    std::unique_ptr<beez::core::IGlobMatcher> matcher;
    std::unique_ptr<beez::core::StepCache> cache;
    static inline int counter = 0;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

}  // namespace

TEST_F(StepCacheTest, StepWithoutArtifactsIsNotCacheable)
{
    beez::core::Step step;
    step.name = "noop";
    step.phase = "generate";
    step.scope = "docs";
    step.shellRun = "true";

    EXPECT_FALSE(isStepCacheable(step));
}

TEST_F(StepCacheTest, StepWithArtifactsIsCacheable)
{
    EXPECT_TRUE(isStepCacheable(makeCompileStep()));
}

TEST_F(StepCacheTest, MissWhenNoEntryExists)
{
    const auto Result = cache->lookup(makeCompileStep(), root, nullptr);
    EXPECT_FALSE(Result.skip);
}

TEST_F(StepCacheTest, HitWhenEntryAndOutputsExist)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    const auto Outputs = std::vector<std::string> {"build/main.o"};
    cache->store(Step, root, nullptr, Outputs);

    const auto Result = cache->lookup(Step, root, nullptr);
    EXPECT_TRUE(Result.skip);
}

TEST_F(StepCacheTest, MissWhenOutputsWereDeleted)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    cache->store(Step, root, nullptr, {"build/main.o"});
    std::filesystem::remove(root / "build" / "main.o");

    const auto Result = cache->lookup(Step, root, nullptr);
    EXPECT_FALSE(Result.skip);
}

TEST_F(StepCacheTest, MissWhenInputContentChanges)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    cache->store(Step, root, nullptr, {"build/main.o"});

    writeFile(root / "src" / "main.cpp", "int main() { return 1; }\n");

    const auto Result = cache->lookup(Step, root, nullptr);
    EXPECT_FALSE(Result.skip);
}

TEST_F(StepCacheTest, MissWhenConfigFingerprintChanges)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    const auto ConfigA = beez::test::makeTestConfig("flags=-O2");
    cache->store(Step, root, ConfigA, {"build/main.o"});

    const auto ConfigB = beez::test::makeTestConfig("flags=-O0");
    const auto Result = cache->lookup(Step, root, ConfigB);
    EXPECT_FALSE(Result.skip);
}

TEST_F(StepCacheTest, CreatesIndexEntryOnStore)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    cache->store(Step, root, nullptr, {"build/main.o"});

    const auto IndexPath = cacheDir / "index" / "compile__compile__cpp.index";
    EXPECT_TRUE(std::filesystem::exists(IndexPath));
}

TEST_F(StepCacheTest, HitViaIndexOnRepeatedLookup)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    cache->store(Step, root, nullptr, {"build/main.o"});

    const auto First = cache->lookup(Step, root, nullptr);
    const auto Second = cache->lookup(Step, root, nullptr);

    ASSERT_TRUE(First.skip);
    ASSERT_TRUE(Second.skip);
    EXPECT_EQ(First.key, Second.key);
}

TEST_F(StepCacheTest, LookupReturnsStoredDurationSeconds)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    constexpr double KDuration = 42.5;
    cache->store(Step, root, nullptr, {"build/main.o"}, KDuration);

    const auto Result = cache->lookup(Step, root, nullptr);
    ASSERT_TRUE(Result.skip);
    EXPECT_DOUBLE_EQ(Result.savedDurationSeconds, KDuration);
}

TEST_F(StepCacheTest, StoreLookupPathPreservesIndexedDuration)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    constexpr double KDuration = 12.25;
    cache->store(Step, root, nullptr, {"build/main.o"}, KDuration);

    const auto Second = cache->lookup(Step, root, nullptr);
    ASSERT_TRUE(Second.skip);
    EXPECT_DOUBLE_EQ(Second.savedDurationSeconds, KDuration);
}

TEST_F(StepCacheTest, MissViaIndexWhenInputSizeChanges)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");
    writeFile(root / "build" / "main.o", "object\n");

    const auto Step = makeCompileStep();
    cache->store(Step, root, nullptr, {"build/main.o"});

    writeFile(root / "src" / "main.cpp", "int main() { return 1; }\n");

    const auto Result = cache->lookup(Step, root, nullptr);
    EXPECT_FALSE(Result.skip);
}

TEST_F(StepCacheTest, TracksOnlyExplicitOutputPaths)
{
    writeFile(root / "build" / "a.o", "a\n");
    writeFile(root / "build" / "noise.bin", "noise\n");

    beez::core::Step step;
    step.name = "compile_a";
    step.phase = "compile";
    step.scope = "cpp";
    step.shellRun = "touch build/a.o";
    step.output = {"build/a.o"};

    beez::core::OutputTracker tracker(root, *matcher);
    tracker.begin(step);
    const auto Outputs = tracker.end(step);

    ASSERT_EQ(Outputs.size(), 1U);
    EXPECT_EQ(Outputs.front(), "build/a.o");
}

TEST_F(StepCacheTest, CapturesBuildDirectoryOutputsWhenNoOutputPatterns)
{
    writeFile(root / "src" / "main.cpp", "int main() {}\n");

    beez::core::Step step;
    step.name = "emit";
    step.phase = "compile";
    step.scope = "cpp";
    step.shellRun = "echo emit";
    step.input = {"src/**/*.cpp"};

    beez::core::OutputTracker tracker(root, *matcher);
    tracker.begin(step);
    writeFile(root / "build" / "generated.o", "object\n");
    const auto Outputs = tracker.end(step);

    ASSERT_EQ(Outputs.size(), 1U);
    EXPECT_EQ(Outputs.front(), "build/generated.o");
}
