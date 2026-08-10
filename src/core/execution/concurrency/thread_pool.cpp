#include "beez/core/execution/concurrency/thread_pool.hpp"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <thread>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

namespace beez::core
{

namespace
{

[[nodiscard]] std::size_t hardwareThreadCount()
{
    const auto Count = std::thread::hardware_concurrency();
    return Count == 0 ? 1 : Count;
}

void pinWorkerThreadIfNeeded(bool enabled)
{
    if (!enabled)
    {
        return;
    }

#ifdef __linux__
    static thread_local bool pinned = false;
    if (pinned)
    {
        return;
    }

    const auto ThreadIndex = tbb::this_task_arena::current_thread_index();
    if (ThreadIndex < 0)
    {
        return;
    }

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(static_cast<int>(ThreadIndex % hardwareThreadCount()), &cpuSet);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet) == 0)
    {
        pinned = true;
    }
#else
    (void)enabled;
#endif
}

}  // namespace

struct ThreadPool::Impl
{
    explicit Impl(int concurrency) : arena_(concurrency) {}

    [[nodiscard]] tbb::task_arena& arena()
    {
        return arena_;
    }

    [[nodiscard]] const tbb::task_arena& arena() const
    {
        return arena_;
    }

  private:
    tbb::task_arena arena_;
};

std::size_t ThreadPool::resolveConcurrency(const std::optional<std::size_t>& requested)
{
    const std::size_t Resolved = requested.value_or(hardwareThreadCount());
    return std::max<std::size_t>(Resolved, 1);
}

ThreadPool::ThreadPool(ThreadPoolConfig config)
    : maxConcurrency_(resolveConcurrency(config.maxThreads)),
      pinThreadsToCores_(config.pinThreadsToCores),
      impl_(std::make_unique<Impl>(static_cast<int>(maxConcurrency_)))
{
}

ThreadPool::~ThreadPool() = default;

void ThreadPool::executeImpl(const std::function<void()>& callback) const
{
    impl_->arena().execute(callback);
}

void ThreadPool::parallelForRange(std::size_t begin,
                                  std::size_t end,
                                  const std::function<void(std::size_t)>& callback) const
{
    const bool PinThreads = pinThreadsToCores_;
    tbb::parallel_for(begin,
                      end,
                      [&](std::size_t index)
                      {
                          pinWorkerThreadIfNeeded(PinThreads);
                          callback(index);
                      });
}

}  // namespace beez::core
