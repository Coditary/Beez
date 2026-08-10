#include "beez/core/orchestrator/run/lifecycle.hpp"

#include "beez/core/model/workflow_step.hpp"
#include "beez/core/orchestrator/run/stats.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>

namespace beez::core
{

double elapsedSeconds(const std::chrono::steady_clock::time_point& start)
{
    const auto End = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(End - start).count();
}

ThroughputRunScope::ThroughputRunScope(plugin::PluginHost& pluginHost, bool optimizeGc)
    : pluginHost_(pluginHost), active_(optimizeGc)
{
    if (active_)
    {
        if (auto* dslLoader = pluginHost_.dslLoader())
        {
            dslLoader->setGcThroughputMode(true);
        }
    }
}

ThroughputRunScope::~ThroughputRunScope()
{
    if (!active_)
    {
        return;
    }

    if (auto* dslLoader = pluginHost_.dslLoader())
    {
        dslLoader->setGcThroughputMode(false);
    }
}

std::string workflowSegmentLabel(const WorkflowStep& step)
{
    return step.invocation.phase + ":" + step.invocation.scope;
}

LoggedRunScope::LoggedRunScope(plugin::PluginHost& pluginHost,
                               bool optimizeGc,
                               RunStatsTracker& stats,
                               logging::ILogger* logger,
                               const std::string& runType,
                               const std::string& name)
    : throughputScope_(pluginHost, optimizeGc), stats_(stats), logger_(logger),
      start_(std::chrono::steady_clock::now())
{
    stats_.reset();
    if (logger_ != nullptr)
    {
        logger_->beginRun(runType, name);
    }
}

void LoggedRunScope::beginSegment(std::string label)
{
    stats_.beginSegment(std::move(label));
}

void LoggedRunScope::endSegment(bool success)
{
    stats_.endSegment(success);
}

logging::RunSummary LoggedRunScope::finish(bool success, std::size_t workerThreads)
{
    const auto Duration = elapsedSeconds(start_);
    if (logger_ != nullptr)
    {
        logger_->endRun(success, Duration, stats_.buildSummary(Duration, workerThreads));
    }
    return stats_.buildSummary(Duration, workerThreads);
}

}  // namespace beez::core
