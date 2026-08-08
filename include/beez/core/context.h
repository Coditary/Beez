#pragma once

#include "beez/core/step_config.hpp"
#include "beez/core/success_cache.hpp"

#include <filesystem>
#include <functional>
#include <optional>

namespace beez::core
{

class WorkerPool;
class SuccessCacheSession;

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

    void setStepIdentity(const StepIdentity& identity);
    void clearStepIdentity();

    [[nodiscard]] const std::optional<StepIdentity>& stepIdentity() const
    {
        return stepIdentity_;
    }

    void setSuccessCacheSession(SuccessCacheSession* session);
    void clearSuccessCacheSession();

    [[nodiscard]] SuccessCacheSession* successCacheSession() const
    {
        return successCacheSession_;
    }

    void setWorkerPool(WorkerPool* pool);
    void clearWorkerPool();

    [[nodiscard]] WorkerPool* workerPool() const
    {
        return workerPool_;
    }

  private:
    std::filesystem::path projectRoot_;
    StepConfigAccessor stepConfigAccessor_;
    std::optional<StepIdentity> stepIdentity_;
    SuccessCacheSession* successCacheSession_ = nullptr;
    WorkerPool* workerPool_ = nullptr;
};

}  // namespace beez::core
