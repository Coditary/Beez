#include "beez/core/thread_pool.hpp"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <thread>

namespace beez::core
{

namespace
{

[[nodiscard]] std::size_t hardwareThreadCount()
{
    const auto Count = std::thread::hardware_concurrency();
    return Count == 0 ? 1 : Count;
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
                                  const std::function<void(std::size_t)>& callback)
{
    tbb::parallel_for(begin, end, [&](std::size_t index) { callback(index); });
}

}  // namespace beez::core
