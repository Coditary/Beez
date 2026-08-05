#include "beez/core/orchestrator.h"

#include "beez/core/expected.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/progress_detail.hpp"
#include "beez/core/run_options.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/stream_capture.hpp"
#include "beez/core/task.hpp"
#include "beez/core/task_action.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/plugin/plugin_host.h"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <future>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] double elapsedSeconds(const std::chrono::steady_clock::time_point& start)
{
    const auto End = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(End - start).count();
}

}  // namespace

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

Orchestrator::Orchestrator(Registry& registry,
                           Context& context,
                           plugin::PluginHost& pluginHost,
                           RunOptions runOptions)
    : registry_(registry), context_(context), pluginHost_(pluginHost), runOptions_(runOptions)
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
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Task", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        ProgressState progress {.total = FoundTask->actions.size()};
        const auto Result = runTask(*FoundTask, progress);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(static_cast<bool>(Result), elapsedSeconds(Start));
        }

        return Result;
    }

    if (const auto FoundWorkflow = registry_.findWorkflow(name))
    {
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->beginRun("Workflow", name);
        }

        const auto Start = std::chrono::steady_clock::now();
        const auto Result = runWorkflow(*FoundWorkflow);
        const auto Duration = elapsedSeconds(Start);

        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(static_cast<bool>(Result), Duration);
        }

        return Result;
    }

    return OrchestratorError::NotFound;
}

void Orchestrator::logProgress(ProgressState& progress,
                               const std::string& category,
                               const std::string& detail) const
{
    if (runOptions_.logger == nullptr)
    {
        return;
    }

    const std::size_t CurrentIndex = progress.index.fetch_add(1) + 1;
    runOptions_.logger->logProgress(logging::ExecutionProgress {
        .index = CurrentIndex,
        .total = progress.total,
        .category = category,
        .detail = detail,
    });
}

Expected<int, OrchestratorError> Orchestrator::runShellCommand(const std::string& command,
                                                               const ProgressLabel& label,
                                                               ProgressState& progress,
                                                               logging::LogChannelId channel)
{
    logProgress(progress, label.category, label.detail);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    auto* executor = pluginHost_.executor();
    if (executor == nullptr)
    {
        return OrchestratorError::ExecutorNotAvailable;
    }

    std::string capturedOutput;
    const int ExitCode = executor->execute(command, context_, &capturedOutput);
    if (runOptions_.outputMode == logging::OutputMode::Verbose && runOptions_.logger != nullptr &&
        !capturedOutput.empty())
    {
        runOptions_.logger->logCommandOutput(channel, capturedOutput);
    }

    if (ExitCode != 0)
    {
        return OrchestratorError::ExecutionFailed;
    }

    return ExitCode;
}

Expected<int, OrchestratorError> Orchestrator::runTask(const Task& task, ProgressState& progress)
{
    int lastExitCode = 0;
    for (const auto& action : task.actions)
    {
        if (const auto* shellAction = std::get_if<TaskShellAction>(&action))
        {
            const auto Result = runShellCommand(
                shellAction->command,
                {.category = "task", .detail = truncateForDisplay(shellAction->command)},
                progress,
                {});
            if (!Result)
            {
                return Result.error();
            }
            lastExitCode = Result.value();
            continue;
        }

        const auto* stepAction = std::get_if<TaskStepAction>(&action);
        if (stepAction == nullptr)
        {
            return OrchestratorError::ExecutionFailed;
        }

        const auto FoundStep = registry_.findStep(stepAction->stepName);
        if (!FoundStep)
        {
            return OrchestratorError::NotFound;
        }

        Step step = *FoundStep;
        step.config = mergeStepConfigs(step.config, stepAction->config);

        const auto Result = runStepInstance(step, progress);
        if (!Result)
        {
            return Result.error();
        }
        lastExitCode = Result.value();
    }

    return lastExitCode;
}

Expected<int, OrchestratorError> Orchestrator::runStepInstance(const Step& step,
                                                               ProgressState& progress)
{
    const std::string Detail = stepProgressDetail(step);
    const std::string Category = step.phase.empty() ? "step" : step.phase;

    if (const auto& command = step.shellRun)
    {
        return runShellCommand(*command, {.category = Category, .detail = Detail}, progress, {});
    }

    logProgress(progress, Category, Detail);

    if (runOptions_.dryRun)
    {
        return 0;
    }

    if (step.hasCallback())
    {
        context_.setStepConfigAccessor([config = step.config]() -> StepConfigPtr
                                       { return config; });

        const auto Captured =
            captureProcessOutput([this, &step]() { return step.callback(context_); });
        context_.clearStepConfigAccessor();

        if (runOptions_.outputMode == logging::OutputMode::Verbose &&
            runOptions_.logger != nullptr && !Captured.output.empty())
        {
            runOptions_.logger->logCommandOutput({}, Captured.output);
        }

        if (Captured.exitCode != 0)
        {
            return OrchestratorError::ExecutionFailed;
        }
        return Captured.exitCode;
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

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->beginRun("Step", name);
    }

    const auto Start = std::chrono::steady_clock::now();
    ProgressState progress {.total = 1};
    const auto Result = runStepInstance(*FoundStep, progress);

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(static_cast<bool>(Result), elapsedSeconds(Start));
    }

    return Result;
}

std::size_t Orchestrator::countPhaseInvocationSteps(const PhaseInvocation& invocation) const
{
    return registry_
        .stepsForPhase(invocation.phase, invocation.scope.empty() ? "*" : invocation.scope)
        .size();
}

std::size_t Orchestrator::countPhaseRequestSteps(const PhaseRequest& request) const
{
    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    return std::accumulate(scopes.begin(),
                           scopes.end(),
                           std::size_t {0},
                           [this, &request](std::size_t total, const std::string& scope)
                           {
                               return total + countPhaseInvocationSteps(PhaseInvocation {
                                                  .phase = request.phase, .scope = scope});
                           });
}

std::size_t Orchestrator::countWorkflowSteps(const Workflow& workflow) const
{
    return std::accumulate(
        workflow.steps.begin(),
        workflow.steps.end(),
        std::size_t {0},
        [this](std::size_t total, const WorkflowStep& step)
        {
            return total +
                   std::accumulate(step.invocations.begin(),
                                   step.invocations.end(),
                                   std::size_t {0},
                                   [this](std::size_t stepTotal, const PhaseInvocation& invocation)
                                   { return stepTotal + countPhaseInvocationSteps(invocation); });
        });
}

Expected<int, OrchestratorError> Orchestrator::runPhaseInvocation(const PhaseInvocation& invocation,
                                                                  ProgressState& progress)
{
    const auto MatchedSteps = registry_.stepsForPhase(
        invocation.phase, invocation.scope.empty() ? "*" : invocation.scope);

    for (const auto& step : MatchedSteps)
    {
        const auto Result = runStepInstance(step, progress);
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

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->beginRun("Phase", request.phase);
    }

    const auto Start = std::chrono::steady_clock::now();

    std::vector<std::string> scopes = request.scopes;
    if (scopes.empty())
    {
        scopes = registry_.scopesForPhase(request.phase);
    }

    if (scopes.empty())
    {
        if (runOptions_.logger != nullptr)
        {
            runOptions_.logger->endRun(true, elapsedSeconds(Start));
        }
        return 0;
    }

    ProgressState progress {.total = countPhaseRequestSteps(request)};

    for (const auto& scope : scopes)
    {
        const PhaseInvocation Invocation {.phase = request.phase, .scope = scope};
        const auto Result = runPhaseInvocation(Invocation, progress);
        if (!Result)
        {
            if (runOptions_.logger != nullptr)
            {
                runOptions_.logger->endRun(false, elapsedSeconds(Start));
            }
            return Result.error();
        }
    }

    if (runOptions_.logger != nullptr)
    {
        runOptions_.logger->endRun(true, elapsedSeconds(Start));
    }

    return 0;
}

Expected<int, OrchestratorError> Orchestrator::runWorkflow(const Workflow& workflow)
{
    ProgressState progress {.total = countWorkflowSteps(workflow)};

    for (const auto& step : workflow.steps)
    {
        if (step.isParallel())
        {
            std::vector<std::future<Expected<int, OrchestratorError>>> futures;
            futures.reserve(step.invocations.size());
            std::vector<logging::LogChannelId> channels;
            channels.reserve(step.invocations.size());

            for (const auto& invocation : step.invocations)
            {
                logging::LogChannelId channel {};
                if (runOptions_.logger != nullptr)
                {
                    channel =
                        runOptions_.logger->openChannel(invocation.phase + ":" + invocation.scope);
                }
                channels.push_back(channel);

                futures.push_back(std::async(std::launch::async,
                                             [this, invocation, channel, &progress]()
                                             { return runPhaseInvocation(invocation, progress); }));
            }

            for (std::size_t index = 0; index < futures.size(); ++index)
            {
                const auto Result = futures.at(index).get();
                if (runOptions_.logger != nullptr)
                {
                    runOptions_.logger->closeChannel(channels.at(index));
                }
                if (!Result)
                {
                    return Result.error();
                }
            }

            continue;
        }

        const auto Result = runPhaseInvocation(step.invocations.front(), progress);
        if (!Result)
        {
            return Result.error();
        }
    }

    return 0;
}

}  // namespace beez::core
