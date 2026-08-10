#pragma once

#include "beez/core/config/ui/types.hpp"

#include <spdlog/logger.h>

#include <filesystem>
#include <memory>

namespace beez::logging
{

[[nodiscard]] std::shared_ptr<spdlog::logger> makeConsoleLogger(const core::UiSettings& uiSettings);

[[nodiscard]] std::shared_ptr<spdlog::logger> makeFileLogger(const std::filesystem::path& logFile,
                                                             const core::UiSettings& uiSettings);

}  // namespace beez::logging
