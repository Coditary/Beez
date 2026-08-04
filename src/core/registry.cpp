#include "beez/core/registry.h"

#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

void Registry::registerTask(Task task)
{
    tasks_.insert_or_assign(task.name, std::move(task));
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

std::optional<Workflow> Registry::findWorkflow(const std::string& name) const
{
    const auto WorkflowIterator = workflows_.find(name);
    if (WorkflowIterator == workflows_.end())
    {
        return std::nullopt;
    }
    return WorkflowIterator->second;
}

std::vector<Task> Registry::tasksForPhase(const std::string& phase, const std::string& scope) const
{
    std::vector<Task> matched;
    for (const auto& [name, task] : tasks_)
    {
        if (!task.phase.has_value() || task.phase.value() != phase)
        {
            continue;
        }

        if (!task.scope.has_value())
        {
            continue;
        }

        if (task.scope.value() == scope || scope == "*")
        {
            matched.push_back(task);
        }
    }
    return matched;
}

}  // namespace beez::core
