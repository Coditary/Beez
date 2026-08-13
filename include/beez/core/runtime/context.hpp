#pragma once

#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace beez::core
{

class GlobMetadataCache;
class WorkerPool;
class SuccessCacheSession;

using CacheStatsRecorder = std::function<void(bool hit, double savedSeconds)>;

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

    void setCacheStatsRecorder(CacheStatsRecorder recorder);
    void clearCacheStatsRecorder();

    void recordCacheUnit(bool hit, double savedSeconds = 0.0) const;

    void setPendingWorkerDuration(double durationSeconds) const;
    [[nodiscard]] double consumePendingWorkerDuration() const;

    void setVerboseOutput(bool verbose);
    [[nodiscard]] bool verboseOutput() const
    {
        return verboseOutput_;
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
    CacheStatsRecorder cacheStatsRecorder_;
    mutable std::optional<double> pendingWorkerDuration_;
    bool verboseOutput_ = false;
};

}  // namespace beez::core
