#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace beez::core
{

struct ThreadPoolConfig
{
    // When unset, concurrency follows hardware thread count.
    std::optional<std::size_t> maxThreads;
};

class ThreadPool
{
  public:
    explicit ThreadPool(ThreadPoolConfig config = {});
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    [[nodiscard]] std::size_t maxConcurrency() const
    {
        return maxConcurrency_;
    }

    [[nodiscard]] bool isSequential() const
    {
        return maxConcurrency_ <= 1;
    }

    template <typename Fn> void execute(Fn&& callback) const
    {
        executeImpl(std::function<void()>(std::forward<Fn>(callback)));
    }

    template <typename Fn> void parallelFor(std::size_t count, Fn&& callback) const
    {
        if (count == 0)
        {
            return;
        }

        if (isSequential() || count == 1)
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                callback(index);
            }
            return;
        }

        const std::function<void(std::size_t)> Task(std::forward<Fn>(callback));
        execute([&] { parallelForRange(0, count, Task); });
    }

  private:
    struct Impl;

    static void parallelForRange(std::size_t begin,
                                 std::size_t end,
                                 const std::function<void(std::size_t)>& callback);

    void executeImpl(const std::function<void()>& callback) const;

    [[nodiscard]] static std::size_t
    resolveConcurrency(const std::optional<std::size_t>& requested);

    std::size_t maxConcurrency_ = 1;
    std::unique_ptr<Impl> impl_;
};

}  // namespace beez::core
