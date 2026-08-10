#pragma once

#include "beez/core/orchestrator/run/stats.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <chrono>
#include <cstddef>
#include <string>

namespace beez::plugin
{
class PluginHost;
}

namespace beez::core
{

struct WorkflowStep;

class ThroughputRunScope
{
  public:
    ThroughputRunScope(plugin::PluginHost& pluginHost, bool optimizeGc);
    ThroughputRunScope(const ThroughputRunScope&) = delete;
    ThroughputRunScope& operator=(const ThroughputRunScope&) = delete;
    ThroughputRunScope(ThroughputRunScope&&) = delete;
    ThroughputRunScope& operator=(ThroughputRunScope&&) = delete;
    ~ThroughputRunScope();

  private:
    plugin::PluginHost& pluginHost_;
    bool active_;
};

[[nodiscard]] std::string workflowSegmentLabel(const WorkflowStep& step);

class LoggedRunScope
{
  public:
    LoggedRunScope(plugin::PluginHost& pluginHost,
                   bool optimizeGc,
                   RunStatsTracker& stats,
                   logging::ILogger* logger,
                   const std::string& runType,
                   const std::string& name);

    void beginSegment(std::string label);
    void endSegment(bool success);

    logging::RunSummary finish(bool success, std::size_t workerThreads);

  private:
    ThroughputRunScope throughputScope_;
    RunStatsTracker& stats_;
    logging::ILogger* logger_;
    std::chrono::steady_clock::time_point start_;
};

}  // namespace beez::core
