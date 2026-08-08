#pragma once

#include "beez/core/ui_options.hpp"
#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"

#include <memory>

namespace beez::logging
{

[[nodiscard]] std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode,
                                                          const core::UiSettings& uiSettings);

}  // namespace beez::logging
