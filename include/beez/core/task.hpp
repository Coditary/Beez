#pragma once

#include "beez/core/task_action.hpp"

#include <string>
#include <vector>

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct Task
{
    std::string name;
    std::vector<TaskAction> actions;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
