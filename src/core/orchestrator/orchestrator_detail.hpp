#pragma once

#include "beez/core/model/workflow_step.hpp"

#include <chrono>
#include <string>

namespace beez::plugin
{
class PluginHost;
}

namespace beez::core::orchestrator_detail
{

[[nodiscard]] double elapsedSeconds(const std::chrono::steady_clock::time_point& start);

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

}  // namespace beez::core::orchestrator_detail
