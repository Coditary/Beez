#include "beez/core/execution/worker_pool.hpp"

#include "beez/core/cache/step_cache.hpp"
#include "beez/core/execution/thread_pool.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] double elapsedSeconds(const std::chrono::steady_clock::time_point& start)
{
    const auto End = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(End - start).count();
}

[[nodiscard]] bool isWorkerCacheable(const WorkerSpec& spec)
{
    return !spec.inputs.empty() || !spec.outputs.empty();
}

[[nodiscard]] std::string joinCommands(const std::vector<std::string>& commands)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < commands.size(); ++index)
    {
        if (index > 0)
        {
            stream << '\n';
        }
        stream << commands.at(index);
    }
    return stream.str();
}

}  // namespace

WorkerPool::WorkerPool(std::filesystem::path projectRoot,
                       ExecuteFn execute,
                       const StepCache* stepCache,
                       const IGlobMatcher& matcher,
                       std::string parentStepName,
                       StepConfigPtr parentStepConfig,
                       // NOLINTNEXTLINE(readability-identifier-naming)
                       const bool dryRun,
                       const ThreadPool* threadPool,
                       CacheStatsRecorder statsRecorder)
    : projectRoot_(std::move(projectRoot)), execute_(std::move(execute)), stepCache_(stepCache),
      matcher_(matcher), parentStepName_(std::move(parentStepName)),
      parentStepConfig_(std::move(parentStepConfig)), dryRun_(dryRun), threadPool_(threadPool),
      statsRecorder_(std::move(statsRecorder))
{
}

WorkerHandle WorkerPool::spawn(WorkerSpec spec)
{
    if (spec.name.empty())
    {
        throw std::invalid_argument("worker name must not be empty");
    }
    if (spec.commands.empty())
    {
        throw std::invalid_argument("worker '" + spec.name + "' must have at least one command");
    }

    const WorkerHandle Handle {.id = workers_.size()};
    workers_.push_back(WorkerEntry {.spec = std::move(spec)});
    return Handle;
}

int WorkerPool::wait(WorkerHandle handle)
{
    return executeWorker(handle.id);
}

int WorkerPool::waitAll(const std::vector<WorkerHandle>& handles)
{
    if (handles.empty())
    {
        return 0;
    }

    if (threadPool_ == nullptr || threadPool_->isSequential() || handles.size() == 1)
    {
        int lastExitCode = 0;
        for (const auto& handle : handles)
        {
            const int ExitCode = executeWorker(handle.id);
            if (ExitCode != 0)
            {
                lastExitCode = ExitCode;
            }
        }
        return lastExitCode;
    }

    int lastExitCode = 0;
    std::mutex exitCodeMutex;
    threadPool_->parallelFor(handles.size(),
                             [&](std::size_t index)
                             {
                                 const int ExitCode = executeWorker(handles.at(index).id);
                                 if (ExitCode != 0)
                                 {
                                     const std::scoped_lock Lock(exitCodeMutex);
                                     lastExitCode = ExitCode;
                                 }
                             });
    return lastExitCode;
}

int WorkerPool::drainAll()
{
    if (workers_.empty())
    {
        return 0;
    }

    if (threadPool_ == nullptr || threadPool_->isSequential())
    {
        int lastExitCode = 0;
        for (std::size_t index = 0; index < workers_.size(); ++index)
        {
            const int ExitCode = executeWorker(index);
            if (ExitCode != 0)
            {
                lastExitCode = ExitCode;
            }
        }
        return lastExitCode;
    }

    int lastExitCode = 0;
    std::mutex exitCodeMutex;
    threadPool_->parallelFor(workers_.size(),
                             [&](std::size_t index)
                             {
                                 const int ExitCode = executeWorker(index);
                                 if (ExitCode != 0)
                                 {
                                     const std::scoped_lock Lock(exitCodeMutex);
                                     lastExitCode = ExitCode;
                                 }
                             });
    return lastExitCode;
}

Step WorkerPool::workerAsStep(const WorkerSpec& spec) const
{
    Step step;
    step.name = spec.name;
    step.phase = "__worker__";
    step.scope = parentStepName_;
    step.input = spec.inputs;
    step.output = spec.outputs;
    step.shellRun = joinCommands(spec.commands);
    return step;
}

int WorkerPool::executeWorker(std::size_t workerId)
{
    if (workerId >= workers_.size())
    {
        throw std::out_of_range("invalid worker handle");
    }

    WorkerEntry& entry = workers_.at(workerId);
    if (entry.done)
    {
        return entry.exitCode;
    }

    if (dryRun_)
    {
        entry.done = true;
        entry.exitCode = 0;
        return 0;
    }

    const Step AsStep = workerAsStep(entry.spec);
    std::optional<OutputTracker> outputTracker;
    if (stepCache_ != nullptr && isWorkerCacheable(entry.spec))
    {
        const auto Lookup = stepCache_->lookup(AsStep, projectRoot_, parentStepConfig_);
        if (Lookup.skip)
        {
            entry.lastDurationSeconds = Lookup.savedDurationSeconds;
            if (Lookup.savedDurationSeconds > 0.0)
            {
                const std::scoped_lock Lock(timingMutex_);
                totalWorkerSavedSeconds_ += Lookup.savedDurationSeconds;
            }

            if (statsRecorder_ != nullptr)
            {
                statsRecorder_(true, Lookup.savedDurationSeconds);
            }

            ++cacheHitCount_;

            entry.done = true;
            entry.exitCode = 0;
            return 0;
        }

        outputTracker.emplace(projectRoot_, matcher_);
        outputTracker->begin(AsStep);
    }

    const auto WorkerStart = std::chrono::steady_clock::now();
    int exitCode = 0;
    for (const auto& command : entry.spec.commands)
    {
        exitCode = execute_(command, entry.spec);
        if (exitCode != 0)
        {
            break;
        }
    }
    const double WorkerDuration = elapsedSeconds(WorkerStart);
    entry.lastDurationSeconds = WorkerDuration;

    entry.done = true;
    entry.exitCode = exitCode;

    if (exitCode == 0)
    {
        const std::scoped_lock Lock(timingMutex_);
        totalWorkerExecutionSeconds_ += WorkerDuration;
    }

    if (statsRecorder_ != nullptr)
    {
        statsRecorder_(false, 0.0);
    }

    ++cacheMissCount_;

    if (exitCode == 0 && outputTracker.has_value() && stepCache_ != nullptr)
    {
        stepCache_->store(
            AsStep, projectRoot_, parentStepConfig_, outputTracker->end(AsStep), WorkerDuration);
    }

    return exitCode;
}

// NOLINTNEXTLINE(readability-identifier-naming)
double WorkerPool::workerDuration(const std::size_t workerId) const
{
    if (workerId >= workers_.size())
    {
        return 0.0;
    }

    return workers_.at(workerId).lastDurationSeconds;
}

double WorkerPool::totalWorkerExecutionSeconds() const
{
    const std::scoped_lock Lock(timingMutex_);
    return totalWorkerExecutionSeconds_;
}

double WorkerPool::totalWorkerSavedSeconds() const
{
    const std::scoped_lock Lock(timingMutex_);
    return totalWorkerSavedSeconds_;
}

std::size_t WorkerPool::cacheHitCount() const
{
    return cacheHitCount_;
}

std::size_t WorkerPool::cacheMissCount() const
{
    return cacheMissCount_;
}

}  // namespace beez::core
