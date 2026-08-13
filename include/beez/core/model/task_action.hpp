#pragma once

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/step_config.hpp"

#include <string>
#include <utility>
#include <variant>

namespace beez::core
{

struct TaskShellAction
{
    std::string command;
};

struct TaskStepAction
{
    std::string stepName;
    StepConfigPtr config;
};

struct TaskInvocationAction
{
    std::string taskName;
};

struct TaskPhaseAction
{
    PhaseInvocation invocation;
};

using TaskAction =
    std::variant<TaskShellAction, TaskStepAction, TaskInvocationAction, TaskPhaseAction>;

[[nodiscard]] inline TaskAction makeShellAction(std::string command)
{
    return TaskShellAction {std::move(command)};
}

[[nodiscard]] inline TaskAction makeStepAction(std::string stepName, StepConfigPtr config = nullptr)
{
    return TaskStepAction {.stepName = std::move(stepName), .config = std::move(config)};
}

[[nodiscard]] inline TaskAction makeTaskInvocation(std::string taskName)
{
    return TaskInvocationAction {.taskName = std::move(taskName)};
}

[[nodiscard]] inline TaskAction makePhaseAction(PhaseInvocation invocation)
{
    return TaskPhaseAction {.invocation = std::move(invocation)};
}

}  // namespace beez::core
