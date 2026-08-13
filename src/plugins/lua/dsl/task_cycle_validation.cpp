#include "beez/plugin/lua/dsl/task_cycle_validation.hpp"

#include "beez/core/model/task_action.hpp"
#include "beez/core/registry/registry.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] bool hasTaskCycleFrom(const std::string& origin,
                                  const std::string& current,
                                  const core::Registry& registry,
                                  std::unordered_set<std::string>& visited)
{
    const auto Found = registry.findTask(current);
    if (!Found.has_value())
    {
        return false;
    }

    for (const auto& action : Found->actions)
    {
        if (const auto* invocation = std::get_if<core::TaskInvocationAction>(&action))
        {
            if (invocation->taskName == origin)
            {
                return true;
            }

            if (!visited.insert(invocation->taskName).second)
            {
                continue;
            }

            if (hasTaskCycleFrom(origin, invocation->taskName, registry, visited))
            {
                return true;
            }

            visited.erase(invocation->taskName);
        }
    }

    return false;
}

}  // namespace

void validateTaskInvocations(const core::Registry& registry)
{
    for (const auto& [taskName, task] : registry.tasks())
    {
        for (const auto& action : task.actions)
        {
            if (const auto* invocation = std::get_if<core::TaskInvocationAction>(&action))
            {
                if (!registry.findTask(invocation->taskName).has_value())
                {
                    throw std::runtime_error("task '" + taskName + "' references undefined task '" +
                                             invocation->taskName + "'");
                }
            }
        }
    }

    for (const auto& [taskName, task] : registry.tasks())
    {
        (void)task;
        std::unordered_set<std::string> visited;
        if (hasTaskCycleFrom(taskName, taskName, registry, visited))
        {
            throw std::runtime_error("task '" + taskName + "' eventually invokes itself");
        }
    }
}

}  // namespace beez::plugin::lua
