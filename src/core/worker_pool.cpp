#include "beez/core/worker_pool.hpp"

#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_cache.hpp"

#include <cstddef>
#include <filesystem>
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
                       bool dryRun)
    : projectRoot_(std::move(projectRoot)), execute_(std::move(execute)), stepCache_(stepCache),
      matcher_(matcher), parentStepName_(std::move(parentStepName)),
      parentStepConfig_(std::move(parentStepConfig)), dryRun_(dryRun)
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

int WorkerPool::drainAll()
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
            entry.done = true;
            entry.exitCode = 0;
            return 0;
        }

        outputTracker.emplace(projectRoot_, matcher_);
        outputTracker->begin(AsStep);
    }

    int exitCode = 0;
    for (const auto& command : entry.spec.commands)
    {
        exitCode = execute_(command);
        if (exitCode != 0)
        {
            break;
        }
    }

    entry.done = true;
    entry.exitCode = exitCode;

    if (exitCode == 0 && outputTracker.has_value() && stepCache_ != nullptr)
    {
        stepCache_->store(AsStep, projectRoot_, parentStepConfig_, outputTracker->end(AsStep));
    }

    return exitCode;
}

}  // namespace beez::core
