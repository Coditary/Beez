#include "beez/core/execution/concurrency/thread_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

namespace
{

[[nodiscard]] bool waitedLongerThan(const std::chrono::steady_clock::duration& threshold)
{
    const auto Start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(threshold);
    const auto End = std::chrono::steady_clock::now();
    return End - Start >= threshold;
}

}  // namespace

TEST(ThreadPoolTest, DefaultUsesAtLeastOneThread)
{
    const beez::core::ThreadPool Pool;
    EXPECT_GE(Pool.maxConcurrency(), 1U);
}

TEST(ThreadPoolTest, ExplicitThreadCountIsHonored)
{
    const beez::core::ThreadPool Pool(beez::core::ThreadPoolConfig {.maxThreads = 4});
    EXPECT_EQ(Pool.maxConcurrency(), 4U);
    EXPECT_FALSE(Pool.isSequential());
}

TEST(ThreadPoolTest, SingleThreadPoolIsSequential)
{
    const beez::core::ThreadPool Pool(beez::core::ThreadPoolConfig {.maxThreads = 1});
    EXPECT_TRUE(Pool.isSequential());
}

TEST(ThreadPoolTest, ParallelForRunsAllIndices)
{
    const beez::core::ThreadPool Pool(beez::core::ThreadPoolConfig {.maxThreads = 4});
    std::vector<int> seen(8, 0);

    Pool.parallelFor(seen.size(),
                     [&seen](std::size_t index) { seen.at(index) = static_cast<int>(index) + 1; });

    for (std::size_t index = 0; index < seen.size(); ++index)
    {
        EXPECT_EQ(seen.at(index), static_cast<int>(index) + 1);
    }
}

TEST(ThreadPoolTest, ParallelForUsesMultipleThreadsWhenAllowed)
{
    const beez::core::ThreadPool Pool(beez::core::ThreadPoolConfig {.maxThreads = 4});
    std::atomic<int> concurrent {0};
    std::atomic<int> peak {0};

    Pool.parallelFor(32,
                     [&](std::size_t /*index*/)
                     {
                         const int Current = concurrent.fetch_add(1) + 1;
                         int observed = peak.load();
                         while (Current > observed &&
                                !peak.compare_exchange_weak(observed, Current))
                         {
                         }

                         EXPECT_TRUE(waitedLongerThan(std::chrono::milliseconds(5)));
                         concurrent.fetch_sub(1);
                     });

    EXPECT_GT(peak.load(), 1);
}

TEST(ThreadPoolTest, SequentialPoolNeverBlocksOnNestedParallelFor)
{
    const beez::core::ThreadPool Pool(beez::core::ThreadPoolConfig {.maxThreads = 1});
    std::atomic<int> executed {0};

    Pool.parallelFor(
        4,
        [&](std::size_t /*index*/)
        { Pool.parallelFor(2, [&](std::size_t /*nested*/) { executed.fetch_add(1); }); });

    EXPECT_EQ(executed.load(), 8);
}
