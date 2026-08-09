#pragma once

#include "beez/core/glob_pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/step_cache.hpp"
#include "beez/core/thread_pool.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace beez::core
{

using CacheStatsRecorder = std::function<void(bool hit, double savedSeconds)>;

struct WorkerSpec
{
    std::string name;
    std::vector<std::string> commands;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct WorkerHandle
{
    std::size_t id = 0;

    [[nodiscard]] bool operator==(const WorkerHandle& other) const
    {
        return id == other.id;
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

class WorkerPool
{
  public:
    using ExecuteFn = std::function<int(const std::string& command, const WorkerSpec& worker)>;

    WorkerPool(std::filesystem::path projectRoot,
               ExecuteFn execute,
               const StepCache* stepCache,
               const IGlobMatcher& matcher,
               std::string parentStepName,
               StepConfigPtr parentStepConfig,
               bool dryRun,
               const ThreadPool* threadPool = nullptr,
               CacheStatsRecorder statsRecorder = nullptr);

    [[nodiscard]] WorkerHandle spawn(WorkerSpec spec);
    [[nodiscard]] int wait(WorkerHandle handle);
    [[nodiscard]] int waitAll(const std::vector<WorkerHandle>& handles);
    [[nodiscard]] int drainAll();

    [[nodiscard]] std::size_t workerCount() const
    {
        return workers_.size();
    }

    [[nodiscard]] double workerDuration(std::size_t workerId) const;

    [[nodiscard]] double totalWorkerExecutionSeconds() const;
    [[nodiscard]] double totalWorkerSavedSeconds() const;
    [[nodiscard]] std::size_t cacheHitCount() const;
    [[nodiscard]] std::size_t cacheMissCount() const;

  private:
    struct WorkerEntry
    {
        WorkerSpec spec;
        bool done = false;
        int exitCode = 0;
        double lastDurationSeconds = 0.0;
    };

    [[nodiscard]] int executeWorker(std::size_t workerId);
    [[nodiscard]] Step workerAsStep(const WorkerSpec& spec) const;

    std::filesystem::path projectRoot_;
    ExecuteFn execute_;
    const StepCache* stepCache_;
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const IGlobMatcher& matcher_;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::string parentStepName_;
    StepConfigPtr parentStepConfig_;
    bool dryRun_;
    const ThreadPool* threadPool_;
    CacheStatsRecorder statsRecorder_;
    std::vector<WorkerEntry> workers_;
    mutable std::mutex timingMutex_;
    double totalWorkerExecutionSeconds_ = 0.0;
    double totalWorkerSavedSeconds_ = 0.0;
    std::size_t cacheHitCount_ = 0;
    std::size_t cacheMissCount_ = 0;
};

}  // namespace beez::core
