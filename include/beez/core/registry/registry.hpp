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
    void registerPluginStep(Step step, const std::string& organization, const std::string& plugin);
    void configureStep(const std::string& name, const StepConfigPtr& config);
    void registerWorkflow(Workflow workflow);
    void registerStepOrder(const std::string& before, const std::string& after);

    void clear();
    void validateConsistent() const;

    [[nodiscard]] std::optional<Task> findTask(const std::string& name) const;
    [[nodiscard]] std::optional<Step> findStep(const std::string& name) const;
    [[nodiscard]] Expected<Step, StepResolutionFailure> resolveStep(const std::string& reference) const;
    [[nodiscard]] std::vector<std::string> stepInvocationNames() const;
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

  private:
    void applyStepConfig(const std::string& name, const StepConfigPtr& config);
    void registerStepAlias(const std::string& alias, const std::string& stepId);
    [[nodiscard]] std::optional<Step> findStepById(const std::string& stepId) const;

    std::unordered_map<std::string, Task> tasks_;
    std::unordered_map<std::string, Step> steps_;
    std::unordered_map<std::string, std::vector<std::string>> stepAliases_;
    std::unordered_map<std::string, StepConfigPtr> pendingStepConfigs_;
    std::unordered_map<std::string, Workflow> workflows_;
    std::vector<StepOrderHint> stepOrderHints_;
};

}  // namespace beez::core
