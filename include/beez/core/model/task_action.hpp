#pragma once

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

using TaskAction = std::variant<TaskShellAction, TaskStepAction>;

[[nodiscard]] inline TaskAction makeShellAction(std::string command)
{
    return TaskShellAction {std::move(command)};
}

[[nodiscard]] inline TaskAction makeStepAction(std::string stepName, StepConfigPtr config = nullptr)
{
    return TaskStepAction {.stepName = std::move(stepName), .config = std::move(config)};
}

}  // namespace beez::core
