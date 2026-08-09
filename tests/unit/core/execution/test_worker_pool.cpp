#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/execution/thread_pool.hpp"
#include "beez/core/execution/worker_pool.hpp"
#include "beez/core/glob/pattern.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

struct CacheStatsTracker
{
    // NOLINTNEXTLINE(readability-identifier-naming) -- mirrors CacheStatsRecorder signature
    void record(const bool hit, const double saved)
    {
        ++totalUnits_;
        if (hit)
        {
            ++cacheHits_;
            savedSeconds_ += saved;
        }
    }

    [[nodiscard]] std::size_t totalUnits() const
    {
        return totalUnits_;
    }

    [[nodiscard]] std::size_t cacheHits() const
    {
        return cacheHits_;
    }

    [[nodiscard]] double savedSeconds() const
    {
        return savedSeconds_;
    }

    [[nodiscard]] beez::core::CacheStatsRecorder callback()
    {
        // NOLINTNEXTLINE(readability-identifier-naming) -- mirrors CacheStatsRecorder signature
        return [this](const bool hit, const double saved) { record(hit, saved); };
    }

  private:
    std::size_t totalUnits_ = 0;
    std::size_t cacheHits_ = 0;
    double savedSeconds_ = 0.0;
};

[[nodiscard]] beez::core::WorkerSpec compileMainWorkerSpec()
{
    return {.name = "compile_main",
            .commands = {"g++ -c src/main.cpp -o build/main.o"},
            .inputs = {"src/main.cpp"},
            .outputs = {"build/main.o"}};
}

[[nodiscard]] beez::core::WorkerPool makeCompileWorkerPool(const std::filesystem::path& projectRoot,
                                                           beez::core::StepCache& cache,
                                                           CacheStatsTracker& tracker)
{
    return {projectRoot,
            [&projectRoot](const std::string& /*command*/,
                           const beez::core::WorkerSpec& /*worker*/) -> int
            {
                writeFile(projectRoot / "build" / "main.o", "fresh-object\n");
                return 0;
            },
            &cache,
            cache.matcher(),
            "compile",
            nullptr,
            false,
            nullptr,
            tracker.callback()};
}

}  // namespace

TEST(WorkerPoolTest, SpawnAndWaitExecutesCommand)
{
    std::vector<std::string> commands;
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&commands](const std::string& command, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            commands.push_back(command);
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false);

    const auto Handle = pool.spawn({.name = "compile_main",
                                    .commands = {"g++ -c main.cpp -o main.o"},
                                    .inputs = {},
                                    .outputs = {}});

    EXPECT_EQ(pool.wait(Handle), 0);
    ASSERT_EQ(commands.size(), 1U);
    EXPECT_EQ(commands[0], "g++ -c main.cpp -o main.o");
}

TEST(WorkerPoolTest, WaitAllExecutesMultipleWorkersSequentially)
{
    std::vector<std::string> commands;
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&commands](const std::string& command, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            commands.push_back(command);
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false);

    const auto First = pool.spawn({.name = "a", .commands = {"cmd-a"}});
    const auto Second = pool.spawn({.name = "b", .commands = {"cmd-b"}});

    EXPECT_EQ(pool.waitAll({First, Second}), 0);
    ASSERT_EQ(commands.size(), 2U);
    EXPECT_EQ(commands[0], "cmd-a");
    EXPECT_EQ(commands[1], "cmd-b");
}

TEST(WorkerPoolTest, WorkerExecutesMultipleCommandsInOrder)
{
    std::vector<std::string> commands;
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&commands](const std::string& command, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            commands.push_back(command);
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false);

    const auto Handle = pool.spawn(
        {.name = "multi", .commands = {"mkdir -p out/", "g++ -c main.cpp", "echo done"}});

    EXPECT_EQ(pool.wait(Handle), 0);
    ASSERT_EQ(commands.size(), 3U);
    EXPECT_EQ(commands[2], "echo done");
}

TEST(WorkerPoolTest, DrainAllExecutesRemainingWorkers)
{
    int executed = 0;
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&executed](const std::string& /*command*/, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            ++executed;
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false);

    (void)pool.spawn({.name = "a", .commands = {"cmd-a"}});
    (void)pool.spawn({.name = "b", .commands = {"cmd-b"}});

    EXPECT_EQ(pool.drainAll(), 0);
    EXPECT_EQ(executed, 2);
}

TEST(WorkerPoolTest, FailedWorkerReturnsNonZeroExitCode)
{
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [](const std::string& /*command*/, const beez::core::WorkerSpec& /*worker*/) -> int
        { return 1; },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false);

    const auto Handle = pool.spawn({.name = "fail", .commands = {"false"}});
    EXPECT_EQ(pool.wait(Handle), 1);
}

TEST(WorkerPoolTest, CachedWorkerSkipsExecutionWhenOutputsExist)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    int executed = 0;
    beez::core::CacheOptions cacheOptions;
    cacheOptions.root = Project.path() / ".cache";
    const beez::core::StepCache Cache(cacheOptions, beez::core::defaultGlobMatcher());
    beez::core::WorkerPool pool(
        Project.path(),
        [&executed, &Project](const std::string& /*command*/,
                              const beez::core::WorkerSpec& /*worker*/) -> int
        {
            ++executed;
            writeFile(Project.path() / "build" / "main.o", "fresh-object\n");
            return 0;
        },
        &Cache,
        Cache.matcher(),
        "compile",
        nullptr,
        false);

    const auto FirstHandle = pool.spawn({.name = "compile_main",
                                         .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                         .inputs = {"src/main.cpp"},
                                         .outputs = {"build/main.o"}});

    EXPECT_EQ(pool.wait(FirstHandle), 0);
    EXPECT_EQ(executed, 1);

    const auto SecondHandle = pool.spawn({.name = "compile_main",
                                          .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                          .inputs = {"src/main.cpp"},
                                          .outputs = {"build/main.o"}});
    EXPECT_EQ(pool.wait(SecondHandle), 0);
    EXPECT_EQ(executed, 1);

    std::filesystem::remove(Project.path() / "build" / "main.o");
    const auto ThirdHandle = pool.spawn({.name = "compile_main",
                                         .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                         .inputs = {"src/main.cpp"},
                                         .outputs = {"build/main.o"}});
    EXPECT_EQ(pool.wait(ThirdHandle), 0);
    EXPECT_EQ(executed, 2);
}

TEST(WorkerPoolTest, DrainAllRunsWorkersInParallelWhenThreadPoolAllows)
{
    const beez::core::ThreadPool ThreadPool(beez::core::ThreadPoolConfig {.maxThreads = 4});
    std::atomic<int> concurrent {0};
    std::atomic<int> peak {0};

    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&concurrent, &peak](const std::string& /*command*/,
                             const beez::core::WorkerSpec& /*worker*/) -> int
        {
            const int Current = concurrent.fetch_add(1) + 1;
            int observed = peak.load();
            while (Current > observed && !peak.compare_exchange_weak(observed, Current))
            {
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            concurrent.fetch_sub(1);
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false,
        &ThreadPool);

    for (int index = 0; index < 8; ++index)
    {
        (void)pool.spawn({.name = "worker-" + std::to_string(index), .commands = {"echo worker"}});
    }

    EXPECT_EQ(pool.drainAll(), 0);
    EXPECT_GT(peak.load(), 1);
}

TEST(WorkerPoolTest, DrainAllStaysSequentialWithSingleThreadPool)
{
    const beez::core::ThreadPool ThreadPool(beez::core::ThreadPoolConfig {.maxThreads = 1});
    std::vector<std::string> commands;

    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&commands](const std::string& command, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            commands.push_back(command);
            return 0;
        },
        nullptr,
        beez::core::defaultGlobMatcher(),
        "parent",
        nullptr,
        false,
        &ThreadPool);

    (void)pool.spawn({.name = "a", .commands = {"cmd-a"}});
    (void)pool.spawn({.name = "b", .commands = {"cmd-b"}});

    EXPECT_EQ(pool.drainAll(), 0);
    ASSERT_EQ(commands.size(), 2U);
    EXPECT_EQ(commands[0], "cmd-a");
    EXPECT_EQ(commands[1], "cmd-b");
}

TEST(WorkerPoolTest, AccumulatesExecutionAndSavedSeconds)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    beez::core::CacheOptions cacheOptions;
    cacheOptions.root = Project.path() / ".cache";
    const beez::core::StepCache Cache(cacheOptions, beez::core::defaultGlobMatcher());
    beez::core::WorkerPool pool(
        Project.path(),
        [&Project](const std::string& /*command*/, const beez::core::WorkerSpec& /*worker*/) -> int
        {
            writeFile(Project.path() / "build" / "main.o", "fresh-object\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 0;
        },
        &Cache,
        Cache.matcher(),
        "compile",
        nullptr,
        false);

    const auto FirstHandle = pool.spawn({.name = "compile_main",
                                         .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                         .inputs = {"src/main.cpp"},
                                         .outputs = {"build/main.o"}});
    EXPECT_EQ(pool.wait(FirstHandle), 0);
    EXPECT_GE(pool.totalWorkerExecutionSeconds(), 0.01);
    EXPECT_DOUBLE_EQ(pool.totalWorkerSavedSeconds(), 0.0);

    const auto CachedHandle = pool.spawn({.name = "compile_main",
                                          .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                          .inputs = {"src/main.cpp"},
                                          .outputs = {"build/main.o"}});
    EXPECT_EQ(pool.wait(CachedHandle), 0);
    EXPECT_GE(pool.totalWorkerSavedSeconds(), 0.01);
    EXPECT_GE(pool.totalWorkerExecutionSeconds() + 1e-6, pool.totalWorkerSavedSeconds());
}

TEST(WorkerPoolTest, RecordsCacheStatsViaCallback)
{
    CacheStatsTracker tracker;

    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    beez::core::CacheOptions cacheOptions;
    cacheOptions.root = Project.path() / ".cache";
    beez::core::StepCache cache(cacheOptions, beez::core::defaultGlobMatcher());
    beez::core::WorkerPool pool = makeCompileWorkerPool(Project.path(), cache, tracker);

    const auto FirstHandle = pool.spawn(compileMainWorkerSpec());
    EXPECT_EQ(pool.wait(FirstHandle), 0);
    EXPECT_EQ(tracker.totalUnits(), 1U);
    EXPECT_EQ(tracker.cacheHits(), 0U);

    const auto CachedHandle = pool.spawn(compileMainWorkerSpec());
    EXPECT_EQ(pool.wait(CachedHandle), 0);
    EXPECT_EQ(tracker.totalUnits(), 2U);
    EXPECT_EQ(tracker.cacheHits(), 1U);
    EXPECT_GT(tracker.savedSeconds(), 0.0);
}

TEST(WorkerPoolTest, CachedWorkerInvalidatesWhenParentConfigChanges)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    int executed = 0;
    beez::core::CacheOptions cacheOptions;
    cacheOptions.root = Project.path() / ".cache";
    const beez::core::StepCache Cache(cacheOptions, beez::core::defaultGlobMatcher());
    beez::core::WorkerPool poolV1(
        Project.path(),
        [&executed, &Project](const std::string& /*command*/,
                              const beez::core::WorkerSpec& /*worker*/) -> int
        {
            ++executed;
            writeFile(Project.path() / "build" / "main.o", "fresh-object\n");
            return 0;
        },
        &Cache,
        Cache.matcher(),
        "compile",
        beez::test::makeTestConfig("tool-v1"),
        false);

    const auto FirstHandle = poolV1.spawn({.name = "compile_main",
                                           .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                           .inputs = {"src/main.cpp"},
                                           .outputs = {"build/main.o"}});
    EXPECT_EQ(poolV1.wait(FirstHandle), 0);
    EXPECT_EQ(executed, 1);

    const auto CachedHandle = poolV1.spawn({.name = "compile_main",
                                            .commands = {"g++ -c src/main.cpp -o build/main.o"},
                                            .inputs = {"src/main.cpp"},
                                            .outputs = {"build/main.o"}});
    EXPECT_EQ(poolV1.wait(CachedHandle), 0);
    EXPECT_EQ(executed, 1);

    beez::core::WorkerPool poolV2(
        Project.path(),
        [&executed, &Project](const std::string& /*command*/,
                              const beez::core::WorkerSpec& /*worker*/) -> int
        {
            ++executed;
            writeFile(Project.path() / "build" / "main.o", "fresh-object\n");
            return 0;
        },
        &Cache,
        Cache.matcher(),
        "compile",
        beez::test::makeTestConfig("tool-v2"),
        false);

    const auto ConfigChangedHandle =
        poolV2.spawn({.name = "compile_main",
                      .commands = {"g++ -c src/main.cpp -o build/main.o"},
                      .inputs = {"src/main.cpp"},
                      .outputs = {"build/main.o"}});
    EXPECT_EQ(poolV2.wait(ConfigChangedHandle), 0);
    EXPECT_EQ(executed, 2);
}
