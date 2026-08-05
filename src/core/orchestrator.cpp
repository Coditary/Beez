#include "beez/core/orchestrator.h"

#include "beez/core/expected.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/plugin/plugin_host.h"

#include <filesystem>
#include <future>
#include <string>
#include <vector>

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

Expected<int, OrchestratorError> Orchestrator::runShellCommand(const std::string& command)
{
    auto* executor = pluginHost_.executor();
    if (executor == nullptr)
    {
        return OrchestratorError::ExecutorNotAvailable;
    }

    const int ExitCode = executor->execute(command, context_);
    if (ExitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return ExitCode;
}

Expected<int, OrchestratorError> Orchestrator::runTask(const Task& task)
{
    int lastExitCode = 0;
    for (const auto& command : task.commands)
    {
        const auto Result = runShellCommand(command);
        if (!Result)
        {
            return Result.error();
        }
        lastExitCode = Result.value();
    }

    return lastExitCode;
}

Expected<int, OrchestratorError> Orchestrator::runStepInstance(const Step& step)
{
    if (const auto& command = step.shellRun)
    {
        return runShellCommand(*command);
    }

    if (step.hasCallback())
    {
        context_.setStepConfigAccessor([config = step.config]() -> StepConfigPtr
                                       { return config; });

        const int ExitCode = step.callback(context_);
        context_.clearStepConfigAccessor();

        if (ExitCode != 0)
        {
            return OrchestratorError::ExecutionFailed;
        }
        return ExitCode;
    }

    return OrchestratorError::ExecutionFailed;
}

Expected<int, OrchestratorError> Orchestrator::runStep(const std::string& name)
{
    const auto FoundStep = registry_.findStep(name);
    if (!FoundStep)
    {
        return OrchestratorError::NotFound;
    }

    return runStepInstance(*FoundStep);
}

Expected<int, OrchestratorError> Orchestrator::runPhaseInvocation(const PhaseInvocation& invocation)
{
    const auto MatchedSteps = registry_.stepsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);

    for (const auto& step : MatchedSteps)
    {
        const auto Result = runStepInstance(step);
        if (!Result)
        {
            return Result.error();
        }
    }

    return 0;
}

Expected<int, OrchestratorError> Orchestrator::runPhase(const PhaseRequest& request)
{
    if (request.phase.empty())
    {
        return OrchestratorError::InvalidPhaseRequest;
    }

    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    if (scopes.empty())
    {
        return 0;
    }

    for (const auto& scope : scopes)
    {
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = runPhaseInvocation(Invocation);
        if (!Result)
        {
            return Result.error();
        }
    }

    return 0;
}

Expected<int, OrchestratorError> Orchestrator::runWorkflow(const Workflow& workflow)
{
    for (const auto& step : workflow.steps)
    {
        if (step.isParallel())
        {
            std::vector<std::future<Expected<int, OrchestratorError>>> futures;
            futures.reserve(step.invocations.size());

            // cppcheck-suppress useStlAlgorithm
            for (const auto& invocation : step.invocations)
            {
                // cppcheck-suppress useStlAlgorithm
                futures.push_back(std::async(std::launch::async,
                                             [this, invocation]()
                                             { return runPhaseInvocation(invocation); }));
            }

            for (auto& future : futures)
            {
                const auto Result = future.get();
                if (!Result)
                {
                    return Result.error();
                }
            }

            continue;
        }

        const auto Result = runPhaseInvocation(step.invocations.front());
        if (!Result)
        {
            return Result.error();
        }
    }

    return 0;
}

}  // namespace beez::core
