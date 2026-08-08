#include "beez/core/glob_pattern.hpp"
#include "beez/core/step_cache.hpp"
#include "beez/core/thread_pool.hpp"
#include "beez/core/worker_pool.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
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

}  // namespace

TEST(WorkerPoolTest, SpawnAndWaitExecutesCommand)
{
    std::vector<std::string> commands;
    beez::core::WorkerPool pool(
        std::filesystem::current_path(),
        [&commands](const std::string& command) -> int
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
        [&commands](const std::string& command) -> int
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
        [&commands](const std::string& command) -> int
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
        [&executed](const std::string& /*command*/) -> int
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
        [](const std::string& /*command*/) -> int { return 1; },
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
    const beez::core::StepCache Cache(Project.path() / ".cache", beez::core::defaultGlobMatcher());
    beez::core::WorkerPool pool(
        Project.path(),
        [&executed, &Project](const std::string& /*command*/) -> int
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
        [&concurrent, &peak](const std::string& /*command*/) -> int
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
        [&commands](const std::string& command) -> int
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

TEST(WorkerPoolTest, CachedWorkerInvalidatesWhenParentConfigChanges)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    int executed = 0;
    const beez::core::StepCache Cache(Project.path() / ".cache", beez::core::defaultGlobMatcher());
    beez::core::WorkerPool poolV1(
        Project.path(),
        [&executed, &Project](const std::string& /*command*/) -> int
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
        [&executed, &Project](const std::string& /*command*/) -> int
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
