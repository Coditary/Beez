#include "beez/core/registry/registry.hpp"

#include "beez/core/glob/pattern.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/registry/step_order.hpp"
#include "beez/core/registry/step_reference.hpp"
#include "beez/core/registry/task_reference.hpp"
#include "beez/core/registry/workflow_reference.hpp"
#include "beez/core/util/expected.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

void mergeAliasTargets(std::vector<std::string>& targets, const std::string& stepId)
{
    if (std::ranges::find(targets, stepId) == targets.end())
    {
        targets.push_back(stepId);
    }
}

}  // namespace

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
    const std::string StepName = step.name;
    const auto PendingIterator = pendingStepConfigs_.find(StepName);
    if (PendingIterator != pendingStepConfigs_.end())
    {
        step.config = mergeStepConfigs(step.config, PendingIterator->second);
        pendingStepConfigs_.erase(PendingIterator);
    }

    if (steps_.contains(StepName))
    {
        throw std::runtime_error("duplicate step name '" + StepName + "'");
    }

    steps_.emplace(StepName, std::move(step));
    registerStepAlias(StepName, StepName);
}

[[nodiscard]] std::string Registry::formatPluginKey(const std::string& organization,
                                                    const std::string& plugin)
{
    return organization + '/' + plugin;
}

[[nodiscard]] bool Registry::stepBelongsToPlugin(const std::string& stepId,
                                                 const std::string& pluginKey)
{
    return stepId.starts_with(pluginKey + ':') || stepId.starts_with(pluginKey + '@');
}

void Registry::applyPluginConfigToRegisteredSteps(const std::string& pluginKey,
                                                  const StepConfigPtr& config)
{
    for (auto& [stepId, step] : steps_)
    {
        if (stepBelongsToPlugin(stepId, pluginKey))
        {
            step.config = mergeStepConfigs(step.config, config);
        }
    }
}

void Registry::registerPluginStep(Step step,
                                  const std::string& organization,
                                  const std::string& plugin,
                                  const std::optional<std::string>& version,
                                  bool allowUnversionedAliases)
{
    const std::string StepName = step.name;
    const std::string StepId =
        version.has_value()
            ? formatVersionedQualifiedStepRef(organization, plugin, *version, StepName)
            : formatQualifiedStepRef(organization, plugin, StepName);

    const std::string PluginKey = formatPluginKey(organization, plugin);
    const auto PluginPendingIterator = pendingPluginConfigs_.find(PluginKey);
    if (PluginPendingIterator != pendingPluginConfigs_.end())
    {
        step.config = mergeStepConfigs(step.config, PluginPendingIterator->second);
    }

    const auto PendingIterator = pendingStepConfigs_.find(StepName);
    if (PendingIterator != pendingStepConfigs_.end())
    {
        step.config = mergeStepConfigs(step.config, PendingIterator->second);
        pendingStepConfigs_.erase(PendingIterator);
    }

    const auto QualifiedPendingIterator = pendingStepConfigs_.find(StepId);
    if (QualifiedPendingIterator != pendingStepConfigs_.end())
    {
        step.config = mergeStepConfigs(step.config, QualifiedPendingIterator->second);
        pendingStepConfigs_.erase(QualifiedPendingIterator);
    }

    if (steps_.contains(StepId))
    {
        throw std::runtime_error("duplicate plugin step '" + StepId + "'");
    }

    steps_.emplace(StepId, std::move(step));
    registerStepAlias(StepId, StepId);

    const bool RegisterUnversionedAliases = !version.has_value() || allowUnversionedAliases;
    if (RegisterUnversionedAliases)
    {
        registerStepAlias(StepName, StepId);
        registerStepAlias(formatShortPluginStepRef(plugin, StepName), StepId);
        registerStepAlias(formatQualifiedStepRef(organization, plugin, StepName), StepId);

        if (isDefaultScopedStepName(StepName))
        {
            const std::string ActionName = stepActionName(StepName);
            registerStepAlias(formatShortPluginStepRef(plugin, ActionName), StepId);
            registerStepAlias(formatQualifiedStepRef(organization, plugin, ActionName), StepId);
        }
    }

    if (version.has_value())
    {
        const std::string& VersionValue = *version;
        registerStepAlias(formatVersionedInvocationRef(StepName, VersionValue), StepId);
        registerStepAlias(
            formatVersionedInvocationRef(formatShortPluginStepRef(plugin, StepName), VersionValue),
            StepId);
        registerStepAlias(formatVersionedInvocationRef(
                              formatQualifiedStepRef(organization, plugin, StepName), VersionValue),
                          StepId);

        if (isDefaultScopedStepName(StepName))
        {
            const std::string ActionName = stepActionName(StepName);
            registerStepAlias(formatVersionedInvocationRef(
                                  formatShortPluginStepRef(plugin, ActionName), VersionValue),
                              StepId);
            registerStepAlias(
                formatVersionedInvocationRef(
                    formatQualifiedStepRef(organization, plugin, ActionName), VersionValue),
                StepId);
        }
    }
}

void Registry::registerStepAlias(const std::string& alias, const std::string& stepId)
{
    auto& targets = stepAliases_[alias];
    mergeAliasTargets(targets, stepId);
}

void Registry::configureStep(const std::string& name, const StepConfigPtr& config)
{
    applyStepConfig(name, config);
}

void Registry::configurePlugin(const std::string& organization,
                               const std::string& plugin,
                               const StepConfigPtr& config)
{
    const std::string PluginKey = formatPluginKey(organization, plugin);
    pendingPluginConfigs_[PluginKey] = mergeStepConfigs(pendingPluginConfigs_[PluginKey], config);
    applyPluginConfigToRegisteredSteps(PluginKey, config);
}

bool Registry::hasPluginSteps(const std::string& organization, const std::string& plugin) const
{
    const std::string PluginKey = formatPluginKey(organization, plugin);
    for (const auto& [stepId, step] : steps_)
    {
        (void)step;
        if (stepBelongsToPlugin(stepId, PluginKey))
        {
            return true;
        }
    }

    return false;
}

void Registry::applyStepConfig(const std::string& name, const StepConfigPtr& config)
{
    const auto StepIterator = steps_.find(name);
    if (StepIterator != steps_.end())
    {
        StepIterator->second.config = mergeStepConfigs(StepIterator->second.config, config);
        return;
    }

    const auto AliasIterator = stepAliases_.find(name);
    if (AliasIterator != stepAliases_.end())
    {
        if (AliasIterator->second.size() > 1U)
        {
            throw std::runtime_error("configure_step reference '" + name + "' is ambiguous");
        }

        if (!AliasIterator->second.empty())
        {
            const auto ResolvedIterator = steps_.find(AliasIterator->second.front());
            if (ResolvedIterator != steps_.end())
            {
                ResolvedIterator->second.config =
                    mergeStepConfigs(ResolvedIterator->second.config, config);
                return;
            }
        }
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

void Registry::registerPluginWorkflow(Workflow workflow,
                                      const std::string& organization,
                                      const std::string& plugin)
{
    const std::string PluginWorkflowKey =
        formatPluginWorkflowKey(organization, plugin, workflow.name);
    if (pluginWorkflows_.contains(PluginWorkflowKey))
    {
        throw std::runtime_error("duplicate plugin workflow '" + PluginWorkflowKey + "'");
    }

    pluginWorkflows_.emplace(PluginWorkflowKey, std::move(workflow));
}

void Registry::registerPluginTask(Task task,
                                  const std::string& organization,
                                  const std::string& plugin)
{
    const std::string PluginTaskKey = formatPluginTaskKey(organization, plugin, task.name);
    if (pluginTasks_.contains(PluginTaskKey))
    {
        throw std::runtime_error("duplicate plugin task '" + PluginTaskKey + "'");
    }

    pluginTasks_.emplace(PluginTaskKey, std::move(task));
}

Workflow Registry::resolvePluginWorkflowReference(const std::string& reference) const
{
    const PluginWorkflowRef ParsedReference = parsePluginWorkflowReference(reference);
    const std::string PluginWorkflowKey = formatPluginWorkflowKey(
        ParsedReference.organization, ParsedReference.plugin, ParsedReference.workflowName);
    const auto WorkflowIterator = pluginWorkflows_.find(PluginWorkflowKey);
    if (WorkflowIterator == pluginWorkflows_.end())
    {
        throw std::runtime_error("plugin workflow reference '" + reference + "' is not defined");
    }

    return WorkflowIterator->second;
}

void Registry::registerWorkflowFromPluginReference(const std::string& localName,
                                                   const std::string& pluginWorkflowReference)
{
    Workflow workflow = resolvePluginWorkflowReference(pluginWorkflowReference);
    workflow.name = localName;
    registerWorkflow(std::move(workflow));
}

Task Registry::resolvePluginTaskReference(const std::string& reference) const
{
    const PluginTaskRef ParsedReference = parsePluginTaskReference(reference);
    const std::string PluginTaskKey = formatPluginTaskKey(
        ParsedReference.organization, ParsedReference.plugin, ParsedReference.taskName);
    const auto TaskIterator = pluginTasks_.find(PluginTaskKey);
    if (TaskIterator == pluginTasks_.end())
    {
        throw std::runtime_error("plugin task reference '" + reference + "' is not defined");
    }

    return TaskIterator->second;
}

void Registry::registerTaskFromPluginReference(const std::string& localName,
                                               const std::string& pluginTaskReference)
{
    Task task = resolvePluginTaskReference(pluginTaskReference);
    task.name = localName;
    registerTask(std::move(task));
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
    stepAliases_.clear();
    pendingStepConfigs_.clear();
    pendingPluginConfigs_.clear();
    workflows_.clear();
    pluginWorkflows_.clear();
    pluginTasks_.clear();
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

std::optional<Step> Registry::findStepById(const std::string& stepId) const
{
    const auto StepIterator = steps_.find(stepId);
    if (StepIterator == steps_.end())
    {
        return std::nullopt;
    }
    return StepIterator->second;
}

std::optional<Step> Registry::findStep(const std::string& name) const
{
    const auto Resolved = resolveStep(name);
    if (!Resolved.hasValue())
    {
        return std::nullopt;
    }

    return Resolved.value();
}

Expected<std::string, StepResolutionFailure>
Registry::resolveStepRegistrationId(const std::string& reference) const
{
    if (steps_.contains(reference))
    {
        return reference;
    }

    const auto AliasIterator = stepAliases_.find(reference);
    if (AliasIterator != stepAliases_.end())
    {
        const auto& targets = AliasIterator->second;
        if (targets.size() > 1U)
        {
            return StepResolutionFailure {.error = StepResolutionError::Ambiguous,
                                          .candidates = targets};
        }

        if (targets.empty())
        {
            return StepResolutionFailure {.error = StepResolutionError::NotFound};
        }

        return targets.front();
    }

    const auto [BaseReference, Version] = splitStepReferenceVersion(reference);

    if (const auto Qualified = parseQualifiedStepRef(reference))
    {
        const auto QualifiedId = Qualified->version.has_value()
                                     ? formatVersionedQualifiedStepRef(Qualified->organization,
                                                                       Qualified->plugin,
                                                                       *Qualified->version,
                                                                       Qualified->stepName)
                                     : formatQualifiedStepRef(Qualified->organization,
                                                              Qualified->plugin,
                                                              Qualified->stepName);
        if (steps_.contains(QualifiedId))
        {
            return QualifiedId;
        }
    }

    if (const auto ShortPlugin = parseShortPluginStepRef(reference))
    {
        const auto ShortAlias =
            Version.has_value()
                ? formatVersionedInvocationRef(
                      formatShortPluginStepRef(ShortPlugin->first, ShortPlugin->second), *Version)
                : formatShortPluginStepRef(ShortPlugin->first, ShortPlugin->second);
        const auto ShortIterator = stepAliases_.find(ShortAlias);
        if (ShortIterator != stepAliases_.end())
        {
            const auto& targets = ShortIterator->second;
            if (targets.size() > 1U)
            {
                return StepResolutionFailure {.error = StepResolutionError::Ambiguous,
                                              .candidates = targets};
            }

            if (!targets.empty())
            {
                return targets.front();
            }
        }
    }

    if (Version.has_value())
    {
        const auto VersionedAlias = formatVersionedInvocationRef(BaseReference, *Version);
        const auto VersionedIterator = stepAliases_.find(VersionedAlias);
        if (VersionedIterator != stepAliases_.end())
        {
            const auto& targets = VersionedIterator->second;
            if (targets.size() == 1U)
            {
                return targets.front();
            }
        }
    }

    return StepResolutionFailure {.error = StepResolutionError::NotFound};
}

Expected<Step, StepResolutionFailure> Registry::resolveStep(const std::string& reference) const
{
    const auto RegistrationId = resolveStepRegistrationId(reference);
    if (!RegistrationId.hasValue())
    {
        return RegistrationId.error();
    }

    if (const auto Found = findStepById(RegistrationId.value()))
    {
        return *Found;
    }

    return StepResolutionFailure {.error = StepResolutionError::NotFound};
}

bool Registry::hasPluginVersionLoaded(const std::string& organization,
                                      const std::string& plugin,
                                      const std::string& version) const
{
    const std::string Prefix = formatPluginVersionKey(organization, plugin, version) + ':';
    for (const auto& [stepId, step] : steps_)
    {
        (void)step;
        if (stepId.starts_with(Prefix))
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string> Registry::stepInvocationNames() const
{
    std::vector<std::string> names;
    names.reserve(stepAliases_.size());

    for (const auto& [alias, targets] : stepAliases_)
    {
        (void)targets;
        names.push_back(alias);
    }

    std::ranges::sort(names);
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
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
    const auto Ordered = orderStepsInLevels(matched, stepOrderHints_, matcher);
    if (!Ordered.hasValue())
    {
        return Ordered.error();
    }

    return isolateCallbackStepsInLevels(Ordered.value());
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
