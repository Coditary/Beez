#include "beez/core/registry.h"

#include "beez/core/expected.hpp"
#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/step_order.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

void Registry::registerTask(Task task)
{
    if (tasks_.contains(task.name))
    {
        throw std::runtime_error("duplicate task name '" + task.name + "'");
    }
    tasks_.emplace(task.name, std::move(task));
}

void Registry::registerStep(Step step)
{
    const auto PendingIterator = pendingStepConfigs_.find(step.name);
    if (PendingIterator != pendingStepConfigs_.end())
    {
        step.config = mergeStepConfigs(step.config, PendingIterator->second);
        pendingStepConfigs_.erase(PendingIterator);
    }

    if (steps_.contains(step.name))
    {
        throw std::runtime_error("duplicate step name '" + step.name + "'");
    }
    steps_.emplace(step.name, std::move(step));
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
    if (workflows_.contains(workflow.name))
    {
        throw std::runtime_error("duplicate workflow name '" + workflow.name + "'");
    }
    workflows_.emplace(workflow.name, std::move(workflow));
}

void Registry::validateConsistent() const
{
    if (pendingStepConfigs_.empty())
    {
        return;
    }

    std::vector<std::string> names;
    names.reserve(pendingStepConfigs_.size());
    for (const auto& [name, config] : pendingStepConfigs_)
    {
        (void)config;
        names.push_back(name);
    }

    // NOLINTNEXTLINE(modernize-use-ranges) -- std::ranges::sort requires additional headers
    std::sort(names.begin(), names.end());

    std::string message = "configure_step referenced undefined step";
    if (names.size() == 1U)
    {
        message += " '" + names.front() + "'";
    }
    else
    {
        message += "s: ";
        bool first = true;
        for (const auto& name : names)
        {
            if (!first)
            {
                message += ", ";
            }
            first = false;
            message += "'" + name + "'";
        }
    }

    throw std::runtime_error(message);
}

void Registry::registerStepOrder(const std::string& before, const std::string& after)
{
    stepOrderHints_.push_back(StepOrderHint {.before = before, .after = after});
}

void Registry::clear()
{
    tasks_.clear();
    steps_.clear();
    pendingStepConfigs_.clear();
    workflows_.clear();
    stepOrderHints_.clear();
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

Expected<std::vector<Step>, StepOrderError> Registry::stepsForPhase(const std::string& phase,
                                                                    const std::string& scope) const
{
    std::vector<Step> matched;
    for (const auto& [name, step] : steps_)
    {
        (void)name;
        if (step.phase != phase)
        {
            continue;
        }

        if (step.scope == scope || scope == "*")
        {
            matched.push_back(step);
        }
    }

    const IGlobMatcher& matcher = defaultGlobMatcher();
    return orderSteps(matched, stepOrderHints_, matcher);
}

Expected<std::vector<std::vector<Step>>, StepOrderError>
Registry::stepLevelsForPhase(const std::string& phase, const std::string& scope) const
{
    std::vector<Step> matched;
    for (const auto& [name, step] : steps_)
    {
        (void)name;
        if (step.phase != phase)
        {
            continue;
        }

        if (step.scope == scope || scope == "*")
        {
            matched.push_back(step);
        }
    }

    const IGlobMatcher& matcher = defaultGlobMatcher();
    return orderStepsInLevels(matched, stepOrderHints_, matcher);
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

std::vector<std::string> Registry::phases() const
{
    std::vector<std::string> phases;
    for (const auto& [name, step] : steps_)
    {
        (void)name;
        if (step.phase.empty())
        {
            continue;
        }

        if (std::ranges::find(phases, step.phase) == phases.end())
        {
            phases.push_back(step.phase);
        }
    }

    // NOLINTNEXTLINE(modernize-use-ranges) -- std::ranges::sort requires additional headers
    std::sort(phases.begin(), phases.end());
    return phases;
}

}  // namespace beez::core
