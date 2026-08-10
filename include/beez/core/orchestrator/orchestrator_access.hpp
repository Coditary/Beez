#pragma once

#include "beez/core/orchestrator/orchestrator.hpp"

namespace beez::core::orchestrator_detail
{

struct Access
{
    static Registry& registry(Orchestrator& orchestrator)
    {
        return orchestrator.registry_;
    }

    static const Registry& registry(const Orchestrator& orchestrator)
    {
        return orchestrator.registry_;
    }

    static Context& context(Orchestrator& orchestrator)
    {
        return orchestrator.context_;
    }

    static const Context& context(const Orchestrator& orchestrator)
    {
        return orchestrator.context_;
    }

    static plugin::PluginHost& pluginHost(Orchestrator& orchestrator)
    {
        return orchestrator.pluginHost_;
    }

    static RunOptions& runOptions(Orchestrator& orchestrator)
    {
        return orchestrator.runOptions_;
    }

    static const RunOptions& runOptions(const Orchestrator& orchestrator)
    {
        return orchestrator.runOptions_;
    }

    static RunStatsTracker& stats(Orchestrator& orchestrator)
    {
        return orchestrator.stats_;
    }

    static CacheWriteCoordinator& cacheWriteCoordinator(Orchestrator& orchestrator)
    {
        return orchestrator.cacheWriteCoordinator_;
    }

    static ThreadPool& threadPool(Orchestrator& orchestrator)
    {
        return orchestrator.threadPool_;
    }
};

}  // namespace beez::core::orchestrator_detail
