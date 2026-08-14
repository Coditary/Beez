#pragma once

#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"

#include <optional>
#include <string>

namespace beez::plugin::lua
{

struct StepPluginEnsureResult
{
    bool success = true;
    std::string message;
};

[[nodiscard]] StepPluginEnsureResult ensureInstalledPluginForStepReference(
    const std::string& reference, core::Registry& registry, const core::Context& context);

}  // namespace beez::plugin::lua
