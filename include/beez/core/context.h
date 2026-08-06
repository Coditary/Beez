#pragma once

#include "beez/core/step_config.hpp"

#include <filesystem>
#include <functional>

namespace beez::core
{

class WorkerPool;

class Context
{
  public:
    using StepConfigAccessor = std::function<StepConfigPtr()>;

    explicit Context(std::filesystem::path projectRoot = std::filesystem::current_path());

    [[nodiscard]] const std::filesystem::path& projectRoot() const
    {
        return projectRoot_;
    }

    [[nodiscard]] std::filesystem::path buildScriptPath() const;

    void setStepConfigAccessor(StepConfigAccessor accessor);
    void clearStepConfigAccessor();

    [[nodiscard]] StepConfigPtr getConfig() const;

    void setWorkerPool(WorkerPool* pool);
    void clearWorkerPool();

    [[nodiscard]] WorkerPool* workerPool() const
    {
        return workerPool_;
    }

  private:
    std::filesystem::path projectRoot_;
    StepConfigAccessor stepConfigAccessor_;
    WorkerPool* workerPool_ = nullptr;
};

}  // namespace beez::core
