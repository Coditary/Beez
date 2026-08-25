#include "beez/core/runtime/context.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/execution/concurrency/worker_pool.hpp"
#include "beez/core/model/step_config.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beez::core
{

Context::Context(std::filesystem::path projectRoot) : projectRoot_(std::move(projectRoot)) {}

std::filesystem::path Context::buildScriptPath() const
{
    return projectRoot_ / buildScriptFileName_.value_or("build.lua");
}

void Context::setBuildScriptFileName(std::string fileName)
{
    buildScriptFileName_ = std::move(fileName);
}

void Context::setEnvFilePath(std::filesystem::path path)
{
    envFilePath_ = std::move(path);
}

void Context::setParameterDefines(std::vector<std::string> defines)
{
    parameterDefines_ = std::move(defines);
}

std::filesystem::path Context::envFilePath() const
{
    if (!envFilePath_.has_value())
    {
        return projectRoot_ / ".env";
    }

    if (envFilePath_->is_relative())
    {
        return projectRoot_ / *envFilePath_;
    }

    return *envFilePath_;
}

void Context::setStepConfigAccessor(StepConfigAccessor accessor)
{
    stepConfigAccessor_ = std::move(accessor);
}

void Context::clearStepConfigAccessor()
{
    stepConfigAccessor_ = nullptr;
}

StepConfigPtr Context::getConfig() const
{
    if (!stepConfigAccessor_)
    {
        return nullptr;
    }

    return stepConfigAccessor_();
}

void Context::setStepIdentity(const StepIdentity& identity)
{
    stepIdentity_ = identity;
}

void Context::clearStepIdentity()
{
    stepIdentity_.reset();
}

void Context::setSuccessCacheSession(SuccessCacheSession* session)
{
    successCacheSession_ = session;
}

void Context::clearSuccessCacheSession()
{
    successCacheSession_ = nullptr;
}

void Context::setWorkerPool(WorkerPool* pool)
{
    workerPool_ = pool;
}

void Context::clearWorkerPool()
{
    workerPool_ = nullptr;
}

void Context::setGlobMetadataCache(GlobMetadataCache* cache)
{
    globMetadataCache_ = cache;
}

void Context::clearGlobMetadataCache()
{
    globMetadataCache_ = nullptr;
}

void Context::setCacheStatsRecorder(CacheStatsRecorder recorder)
{
    cacheStatsRecorder_ = std::move(recorder);
}

void Context::clearCacheStatsRecorder()
{
    cacheStatsRecorder_ = nullptr;
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Context::recordCacheUnit(const bool hit, const double savedSeconds) const
{
    if (cacheStatsRecorder_ != nullptr)
    {
        cacheStatsRecorder_(hit, savedSeconds);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void Context::setPendingWorkerDuration(const double durationSeconds) const
{
    pendingWorkerDuration_ = durationSeconds;
}

double Context::consumePendingWorkerDuration() const
{
    if (!pendingWorkerDuration_.has_value())
    {
        return 0.0;
    }

    const double Duration = *pendingWorkerDuration_;
    pendingWorkerDuration_.reset();
    return Duration;
}

void Context::setVerboseOutput(const bool Verbose)
{
    verboseOutput_ = Verbose;
}

void Context::setFailureLogCallback(FailureLogCallback callback)
{
    failureLogCallback_ = std::move(callback);
}

void Context::clearFailureLogCallback()
{
    failureLogCallback_ = nullptr;
}

void Context::logFailure(const std::string_view Message) const
{
    if (failureLogCallback_ != nullptr)
    {
        failureLogCallback_(Message);
    }
}

}  // namespace beez::core
