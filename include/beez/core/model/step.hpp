#pragma once

#include "beez/core/model/step_config.hpp"
#include "beez/core/runtime/context.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

using StepCallback = std::function<int(const Context&)>;

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct Step
{
    std::string name;
    std::string phase;
    std::string scope;
    std::optional<std::string> description;
    std::optional<std::string> shellRun;
    StepCallback callback;
    StepConfigPtr config;
    std::vector<std::string> input;
    std::vector<std::string> output;
    std::vector<std::string> mutate;

    [[nodiscard]] bool hasConfig() const
    {
        return config != nullptr && !config->empty();
    }

    [[nodiscard]] bool hasShellRun() const
    {
        return shellRun.has_value();
    }

    [[nodiscard]] bool hasCallback() const
    {
        return static_cast<bool>(callback);
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace beez::core
