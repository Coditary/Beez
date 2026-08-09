#include "orchestrator_detail.hpp"

#include "beez/core/model/workflow_step.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <chrono>
#include <string>

namespace beez::core::orchestrator_detail
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

}  // namespace beez::core::orchestrator_detail
