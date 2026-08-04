#include "beez/core/orchestrator.h"

#include "beez/core/expected.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/plugin/plugin_host.h"

#include <filesystem>
#include <string>

namespace beez::core
{

const char* toString(OrchestratorError error)
{
    switch (error)
    {
    case OrchestratorError::NotFound:
        return "name not found in registry";
    case OrchestratorError::WorkflowNotImplemented:
        return "workflow execution is not yet implemented";
    case OrchestratorError::ExecutionFailed:
        return "task execution failed";
    case OrchestratorError::BuildScriptNotFound:
        return "build.lua not found";
    case OrchestratorError::BuildScriptLoadFailed:
        return "failed to load build.lua";
    case OrchestratorError::ExecutorNotAvailable:
        return "no shell executor plugin available";
    }
    return "unknown orchestrator error";
}

Orchestrator::Orchestrator(Registry& registry, Context& context, plugin::PluginHost& pluginHost)
    : registry_(registry), context_(context), pluginHost_(pluginHost)
{
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

Expected<int, OrchestratorError> Orchestrator::run(const std::string& name)
{
    if (const auto FoundTask = registry_.findTask(name))
    {
        return runTask(*FoundTask);
    }

    if (const auto FoundWorkflow = registry_.findWorkflow(name))
    {
        return runWorkflow(*FoundWorkflow);
    }

    return OrchestratorError::NotFound;
}

Expected<int, OrchestratorError> Orchestrator::runTask(const Task& task)
{
    auto* executor = pluginHost_.executor();
    if (executor == nullptr)
    {
        return OrchestratorError::ExecutorNotAvailable;
    }

    const int ExitCode = executor->execute(task.run, context_);
    return ExitCode;
}

// cppcheck-suppress functionStatic
Expected<int, OrchestratorError> Orchestrator::runWorkflow(const Workflow& /*workflow*/)
{
    return OrchestratorError::WorkflowNotImplemented;
}

}  // namespace beez::core
