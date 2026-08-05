#include "beez/core/registry.h"

#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

void Registry::registerTask(Task task)
{
    tasks_.insert_or_assign(task.name, std::move(task));
}

void Registry::registerStep(Step step)
{
    const auto PendingIterator = pendingStepConfigs_.find(step.name);
    if (PendingIterator != pendingStepConfigs_.end())
    {
        step.config = mergeStepConfigs(PendingIterator->second, step.config);
        pendingStepConfigs_.erase(PendingIterator);
    }

    steps_.insert_or_assign(step.name, std::move(step));
}

void Registry::configureStep(const std::string& name, const StepConfigPtr& config)
{
    applyStepConfig(name, config);
}

void Registry::applyStepConfig(const std::string& name, const StepConfigPtr& config)
{
    const auto StepIterator = steps_.find(name);
    if (StepIterator != steps_.end())
    {
        StepIterator->second.config = mergeStepConfigs(StepIterator->second.config, config);
        return;
    }

    pendingStepConfigs_[name] = mergeStepConfigs(pendingStepConfigs_[name], config);
}

void Registry::registerWorkflow(Workflow workflow)
{
    workflows_.insert_or_assign(workflow.name, std::move(workflow));
}

std::optional<Task> Registry::findTask(const std::string& name) const
{
    const auto TaskIterator = tasks_.find(name);
    if (TaskIterator == tasks_.end())
    {
        return std::nullopt;
    }
    return TaskIterator->second;
}

std::optional<Step> Registry::findStep(const std::string& name) const
{
    const auto StepIterator = steps_.find(name);
    if (StepIterator == steps_.end())
    {
        return std::nullopt;
    }
    return StepIterator->second;
}

std::optional<Workflow> Registry::findWorkflow(const std::string& name) const
{
    const auto WorkflowIterator = workflows_.find(name);
    if (WorkflowIterator == workflows_.end())
    {
        return std::nullopt;
    }
    return WorkflowIterator->second;
}

std::vector<Step> Registry::stepsForPhase(const std::string& phase, const std::string& scope) const
{
    std::vector<Step> matched;
    for (const auto& [name, step] : steps_)
    {
        if (step.phase != phase)
        {
            continue;
        }

        if (step.scope == scope || scope == "*")
        {
            matched.push_back(step);
        }
    }
    return matched;
}

std::vector<std::string> Registry::scopesForPhase(const std::string& phase) const
{
    std::vector<std::string> scopes;
    for (const auto& [name, step] : steps_)
    {
        if (step.phase != phase)
        {
            continue;
        }

        if (std::ranges::find(scopes, step.scope) == scopes.end())
        {
            scopes.push_back(step.scope);
        }
    }

    // NOLINTNEXTLINE(modernize-use-ranges) -- std::ranges::sort requires additional headers
    std::sort(scopes.begin(), scopes.end());
    return scopes;
}

}  // namespace beez::core
