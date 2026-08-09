#pragma once

#include "beez/core/workflow.hpp"

#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] core::Workflow parseWorkflow(const std::string& name, const sol::table& stepsTable);

}  // namespace beez::plugin::lua
