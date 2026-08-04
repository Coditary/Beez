#pragma once

#include <string>
#include <vector>

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct Task
{
    std::string name;
    std::vector<std::string> commands;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
