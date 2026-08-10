#pragma once

#include "beez/core/model/phase_request.hpp"

#include <optional>
#include <string>

namespace beez::core
{

[[nodiscard]] std::optional<PhaseRequest> parsePhaseArgument(const std::string& input);

}  // namespace beez::core
