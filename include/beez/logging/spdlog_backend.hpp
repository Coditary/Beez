#pragma once

#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"

#include <memory>

namespace beez::logging
{

[[nodiscard]] std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode);

}  // namespace beez::logging
