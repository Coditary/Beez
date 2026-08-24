#pragma once

#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/registry/step_order.hpp"
#include "beez/core/registry/step_resolution.hpp"
#include "beez/core/util/expected.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::core
{

class Registry
{
  public:
    void registerTask(Task task);
    void registerStep(Step step);
    void registerPluginStep(Step step,
                            const std::string& organization,
                            const std::string& plugin,
                            const std::optional<std::string>& version = std::nullopt,
                            bool allowUnversionedAliases = true);
    void configureStep(const std::string& name, const StepConfigPtr& config);
    void configurePlugin(const std::string& organization,
                         const std::string& plugin,
                         const StepConfigPtr& config);
    void registerWorkflow(Workflow workflow);
    void registerPluginWorkflow(Workflow workflow,
                                const std::string& organization,
                                const std::string& plugin);
    void registerPluginTask(Task task, const std::string& organization, const std::string& plugin);
    [[nodiscard]] Workflow resolvePluginWorkflowReference(const std::string& reference) const;
    [[nodiscard]] std::optional<Workflow>
    tryResolvePluginWorkflowReference(const std::string& reference) const;
    void resolvePendingWorkflowReferences();
    [[nodiscard]] bool hasPendingWorkflowReferences() const;
    [[nodiscard]] std::vector<std::string> pendingWorkflowReferenceNames() const;
    [[nodiscard]] Task resolvePluginTaskReference(const std::string& reference) const;
    void registerWorkflowFromPluginReference(const std::string& localName,
                                             const std::string& pluginWorkflowReference);
    void registerTaskFromPluginReference(const std::string& localName,
                                         const std::string& pluginTaskReference);
    void registerStepOrder(const std::string& before, const std::string& after);

    void clear();
    void validateConsistent() const;

    [[nodiscard]] std::optional<Task> findTask(const std::string& name) const;
    [[nodiscard]] std::optional<Step> findStep(const std::string& name) const;
    [[nodiscard]] Expected<Step, StepResolutionFailure>
    resolveStep(const std::string& reference) const;
    [[nodiscard]] Expected<std::string, StepResolutionFailure>
    resolveStepRegistrationId(const std::string& reference) const;
    [[nodiscard]] std::vector<std::string> stepInvocationNames() const;
    [[nodiscard]] bool hasPluginVersionLoaded(const std::string& organization,
                                              const std::string& plugin,
                                              const std::string& version) const;
    [[nodiscard]] std::optional<Workflow> findWorkflow(const std::string& name) const;
    [[nodiscard]] Expected<std::vector<Step>, StepOrderError>
    stepsForPhase(const std::string& phase, const std::string& scope) const;
    [[nodiscard]] Expected<std::vector<std::vector<Step>>, StepOrderError>
    stepLevelsForPhase(const std::string& phase, const std::string& scope) const;
    [[nodiscard]] std::vector<std::string> scopesForPhase(const std::string& phase) const;
    [[nodiscard]] std::vector<std::string> phases() const;

    [[nodiscard]] const std::unordered_map<std::string, Task>& tasks() const
    {
        return tasks_;
    }

    [[nodiscard]] const std::unordered_map<std::string, Step>& steps() const
    {
        return steps_;
    }

    [[nodiscard]] const std::unordered_map<std::string, Workflow>& workflows() const
    {
        return workflows_;
    }

    [[nodiscard]] bool hasPluginSteps(const std::string& organization,
                                      const std::string& plugin) const;

    [[nodiscard]] const std::unordered_map<std::string, StepConfigPtr>&
    configuredPluginConfigs() const
    {
        return pendingPluginConfigs_;
    }

    [[nodiscard]] const std::unordered_map<std::string, Workflow>& pluginWorkflows() const
    {
        return pluginWorkflows_;
    }

    [[nodiscard]] const std::unordered_map<std::string, Task>& pluginTasks() const
    {
        return pluginTasks_;
    }

  private:
    void applyStepConfig(const std::string& name, const StepConfigPtr& config);
    void applyPluginConfigToRegisteredSteps(const std::string& pluginKey,
                                            const StepConfigPtr& config);
    [[nodiscard]] static std::string formatPluginKey(const std::string& organization,
                                                     const std::string& plugin);
    [[nodiscard]] static bool stepBelongsToPlugin(const std::string& stepId,
                                                  const std::string& pluginKey);
    void registerStepAlias(const std::string& alias, const std::string& stepId);
    [[nodiscard]] std::optional<Step> findStepById(const std::string& stepId) const;

    std::unordered_map<std::string, Task> tasks_;
    std::unordered_map<std::string, Step> steps_;
    std::unordered_map<std::string, std::vector<std::string>> stepAliases_;
    std::unordered_map<std::string, StepConfigPtr> pendingStepConfigs_;
    std::unordered_map<std::string, StepConfigPtr> pendingPluginConfigs_;
    std::unordered_map<std::string, Workflow> workflows_;
    std::unordered_map<std::string, Workflow> pluginWorkflows_;
    std::unordered_map<std::string, Task> pluginTasks_;
    std::unordered_map<std::string, std::string> pendingWorkflowReferences_;
    std::vector<StepOrderHint> stepOrderHints_;
};

}  // namespace beez::core
