#pragma once

#include "beez/core/model/phase_invocation.hpp"

#include <string>

namespace beez::core
{

[[nodiscard]] PhaseInvocation parsePhaseScopeReference(const std::string& reference);

}  // namespace beez::core
