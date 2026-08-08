#include "beez/core/context.h"
#include "beez/core/step_config.hpp"
#include "beez/core/success_cache.hpp"
#include "beez/core/worker_pool.hpp"

#include <filesystem>
#include <utility>

namespace beez::core
{

Context::Context(std::filesystem::path projectRoot) : projectRoot_(std::move(projectRoot)) {}

std::filesystem::path Context::buildScriptPath() const
{
    return projectRoot_ / "build.lua";
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

}  // namespace beez::core
