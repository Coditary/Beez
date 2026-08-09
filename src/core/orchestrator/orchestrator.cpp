#include "beez/core/orchestrator/orchestrator.hpp"
#include "orchestrator_detail.hpp"

#include "beez/core/cache/step/step_cache.hpp"
#include "beez/core/cache/success/success_cache.hpp"
#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"
#include "beez/core/config/run_options.hpp"
#include "beez/core/execution/thread_pool.hpp"
#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/util/expected.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace beez::core
{

const char* toString(OrchestratorError error)
{
    switch (error)
    {
    case OrchestratorError::NotFound:
        return "name not found in registry";
    case OrchestratorError::ExecutionFailed:
        return "task execution failed";
    case OrchestratorError::BuildScriptNotFound:
        return "build.lua not found";
    case OrchestratorError::BuildScriptLoadFailed:
        return "failed to load build.lua";
    case OrchestratorError::ExecutorNotAvailable:
        return "no shell executor plugin available";
    case OrchestratorError::InvalidPhaseRequest:
        return "invalid phase request";
    case OrchestratorError::StepOrderingFailed:
        return "step ordering failed";
    }
    return "unknown orchestrator error";
}

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
    flushBufferedCacheWrites();
    context_.clearGlobMetadataCache();
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

void Orchestrator::flushBufferedCacheWrites()
{
    cacheWriteCoordinator_.flush(runOptions_.cache);
}

void Orchestrator::flushBufferedCacheWritesForPhase()
{
    if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::Phase)
    {
        flushBufferedCacheWrites();
    }
}

Expected<int, OrchestratorError> Orchestrator::run(const std::string& name)
{
    const orchestrator_detail::ThroughputRunScope ThroughputScope(
        pluginHost_, runOptions_.performance.optimizeGcForThroughput);
    const auto FlushAtRunEnd = [this](const auto& result)
    {
        if (runOptions_.performance.cacheWriteStrategy == CacheWriteStrategy::End)
        {
            flushBufferedCacheWrites();
        }
        return result;
    };
    if (const auto FoundTask = registry_.findTask(name))
    {
        resetRunStats();
        beginRunSegment(name);
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Task", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        ProgressState progress {.total = FoundTask->actions.size()};
        const auto Result = runTask(*FoundTask, progress);
        endRunSegment(static_cast<bool>(Result));

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(static_cast<bool>(Result),
                                       orchestrator_detail::elapsedSeconds(Start),
                                       buildRunSummary(orchestrator_detail::elapsedSeconds(Start)));
        }

        return FlushAtRunEnd(Result);
    }

    if (const auto FoundWorkflow = registry_.findWorkflow(name))
    {
        resetRunStats();
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Workflow", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        const auto Result = runWorkflow(*FoundWorkflow);
        const auto Duration = orchestrator_detail::elapsedSeconds(Start);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(
                static_cast<bool>(Result), Duration, buildRunSummary(Duration));
        }

        return FlushAtRunEnd(Result);
    }

    return OrchestratorError::NotFound;
}

}  // namespace beez::core
