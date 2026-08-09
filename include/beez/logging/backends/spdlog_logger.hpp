#pragma once

#include "beez/core/config/ui_options.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/contract/logger.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <memory>

namespace beez::logging
{

[[nodiscard]] std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode,
                                                          const core::UiSettings& uiSettings,
                                                          const LoggingSettings& loggingSettings);

}  // namespace beez::logging
