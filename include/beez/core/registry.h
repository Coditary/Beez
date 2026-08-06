#pragma once

#include "beez/core/expected.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/step_order.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

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
    void configureStep(const std::string& name, const StepConfigPtr& config);
    void registerWorkflow(Workflow workflow);
    void registerStepOrder(const std::string& before, const std::string& after);

    [[nodiscard]] std::optional<Task> findTask(const std::string& name) const;
    [[nodiscard]] std::optional<Step> findStep(const std::string& name) const;
    [[nodiscard]] std::optional<Workflow> findWorkflow(const std::string& name) const;
    [[nodiscard]] Expected<std::vector<Step>, StepOrderError>
    stepsForPhase(const std::string& phase, const std::string& scope) const;
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

    std::unordered_map<std::string, Task> tasks_;
    std::unordered_map<std::string, Step> steps_;
    // TODO(step-config): After build.lua is fully loaded, warn on pending entries that never
    // matched a registered step.
    std::unordered_map<std::string, StepConfigPtr> pendingStepConfigs_;
    std::unordered_map<std::string, Workflow> workflows_;
    std::vector<StepOrderHint> stepOrderHints_;
};

}  // namespace beez::core
