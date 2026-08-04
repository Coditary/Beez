#pragma once

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
    void registerWorkflow(Workflow workflow);

    [[nodiscard]] std::optional<Task> findTask(const std::string& name) const;
    [[nodiscard]] std::optional<Workflow> findWorkflow(const std::string& name) const;
    [[nodiscard]] std::vector<Task> tasksForPhase(const std::string& phase,
                                                  const std::string& scope) const;

    [[nodiscard]] const std::unordered_map<std::string, Task>& tasks() const
    {
        return tasks_;
    }

    [[nodiscard]] const std::unordered_map<std::string, Workflow>& workflows() const
    {
        return workflows_;
    }

  private:
    std::unordered_map<std::string, Task> tasks_;
    std::unordered_map<std::string, Workflow> workflows_;
};

}  // namespace beez::core
