#pragma once

#include "beez/core/registry.h"

#include <string>

namespace beez::cli
{

[[nodiscard]] int runListCommand(const core::Registry& registry, const std::string& kind);

}  // namespace beez::cli
