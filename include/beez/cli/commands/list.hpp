#pragma once

#include "beez/core/registry/registry.hpp"

#include <string>

namespace beez::cli
{

[[nodiscard]] int runListCommand(const core::Registry& registry, const std::string& kind);

}  // namespace beez::cli
