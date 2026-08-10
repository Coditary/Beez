#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/orchestrator/orchestrator_access.hpp"
#include "beez/core/orchestrator/orchestrator_internal.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/config/performance/performance_options.hpp"
#include "beez/core/config/settings/run_options.hpp"
#include "beez/core/execution/concurrency/thread_pool.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace beez::core
{

Orchestrator::Orchestrator(Registry& registry,
                           Context& context,
                           plugin::PluginHost& pluginHost,
                           const RunOptions& runOptions)
    : registry_(registry), context_(context), pluginHost_(pluginHost), runOptions_(runOptions),
      cacheWriteCoordinator_(runOptions.performance.cacheWriteStrategy),
      globMetadataCache_(runOptions.performance.cacheFilesystemMetadata),
      threadPool_(ThreadPoolConfig {.maxThreads = runOptions.maxThreads,
                                    .pinThreadsToCores = runOptions.performance.pinThreadsToCores})
{
    if (runOptions_.enableCache)
    {
        auto cacheOptions = runOptions_.cache;
        if (cacheOptions.root.empty())
        {
            cacheOptions.root = context.projectRoot() / ".cache";
        }

        cacheOptions.writeCoordinator = &cacheWriteCoordinator_;
        runOptions_.cache = cacheOptions;

        if (runOptions_.stepCache == nullptr)
        {
            ownedStepCache_ = std::make_unique<StepCache>(
                cacheOptions, defaultGlobMatcher(), &globMetadataCache_);
            runOptions_.stepCache = ownedStepCache_.get();
        }

        if (runOptions_.successCache == nullptr)
        {
            ownedSuccessCache_ = std::make_unique<SuccessCache>(cacheOptions, defaultGlobMatcher());
            runOptions_.successCache = ownedSuccessCache_.get();
        }
    }

    if (globMetadataCache_.enabled())
    {
        globMetadataCache_.clear();
        context_.setGlobMetadataCache(&globMetadataCache_);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    context_.setCacheStatsRecorder([this](const bool hit, const double savedSeconds)
                                   { recordCacheUnit(hit, savedSeconds); });
}

Orchestrator::~Orchestrator()
{
    orchestrator_detail::flushBufferedCacheWrites(*this);
    context_.clearGlobMetadataCache();
}

Context& Orchestrator::context()
{
    return context_;
}

const Context& Orchestrator::context() const
{
    return context_;
}

plugin::PluginHost& Orchestrator::pluginHost()
{
    return pluginHost_;
}

std::size_t Orchestrator::workerThreads() const
{
    return threadPool_.maxConcurrency();
}

Expected<void, OrchestratorError> Orchestrator::loadBuildScript()
{
    const auto ScriptPath = context_.buildScriptPath();
    if (!std::filesystem::exists(ScriptPath))
    {
        return OrchestratorError::BuildScriptNotFound;
    }

    auto* dslLoader = pluginHost_.dslLoader();
    if (dslLoader == nullptr || !dslLoader->load(context_, registry_))
    {
        return OrchestratorError::BuildScriptLoadFailed;
    }

    return {};
}

void Orchestrator::recordCacheUnit(bool hit, double savedSeconds)
{
    stats_.recordCacheUnit(hit, savedSeconds);
}

void Orchestrator::recordCacheBulk(std::size_t totalUnits, std::size_t hits, double savedSeconds)
{
    stats_.recordCacheBulk(totalUnits, hits, savedSeconds);
}

void Orchestrator::recordPeakWorkers(std::size_t workerCount)
{
    stats_.recordPeakWorkers(workerCount);
}

Expected<int, OrchestratorError> Orchestrator::run(const std::string& name)
{
    if (const auto FoundTask = orchestrator_detail::Access::registry(*this).findTask(name))
    {
        return orchestrator_detail::ScopedLoggedRun(
                   *this, "Task", name, orchestrator_detail::RunCacheFlushPolicy::IfEndStrategy)
            .withSegment(name,
                         [&]
                         {
                             ProgressState progress {.total = FoundTask->actions.size()};
                             return orchestrator_detail::runTask(*this, *FoundTask, progress);
                         });
    }

    if (const auto FoundWorkflow = orchestrator_detail::Access::registry(*this).findWorkflow(name))
    {
        return orchestrator_detail::ScopedLoggedRun(
                   *this, "Workflow", name, orchestrator_detail::RunCacheFlushPolicy::IfEndStrategy)
            .withoutSegment([&] { return orchestrator_detail::runWorkflow(*this, *FoundWorkflow); });
    }

    return OrchestratorError::NotFound;
}

}  // namespace beez::core
