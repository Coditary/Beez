#pragma once

#include "beez/core/step_config.hpp"
#include "beez/core/success_cache.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace beez::core
{

class GlobMetadataCache;
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

    void setBuildScriptFileName(std::string fileName);
    void setEnvFilePath(std::filesystem::path envFilePath);

    [[nodiscard]] std::filesystem::path envFilePath() const;

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

    void setGlobMetadataCache(GlobMetadataCache* cache);
    void clearGlobMetadataCache();

    [[nodiscard]] GlobMetadataCache* globMetadataCache() const
    {
        return globMetadataCache_;
    }

  private:
    std::filesystem::path projectRoot_;
    std::optional<std::string> buildScriptFileName_;
    std::optional<std::filesystem::path> envFilePath_;
    StepConfigAccessor stepConfigAccessor_;
    std::optional<StepIdentity> stepIdentity_;
    SuccessCacheSession* successCacheSession_ = nullptr;
    WorkerPool* workerPool_ = nullptr;
    GlobMetadataCache* globMetadataCache_ = nullptr;
};

}  // namespace beez::core
