#pragma once

#include <optional>
#include <string>

namespace beez::core
{

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct Task
{
    std::string name;
    std::string run;
    std::optional<std::string> phase;
    std::optional<std::string> scope;

    [[nodiscard]] bool isOrphan() const
    {
        return !phase.has_value();
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
