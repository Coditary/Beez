#pragma once

#include "beez/core/config/cache_options.hpp"
#include "beez/core/config/performance_options.hpp"
#include "beez/core/config/ui_options.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/persistence/run_log_writer.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <cstddef>
#include <optional>

namespace beez::logging
{
class ILogger;
}  // namespace beez::logging

namespace beez::core
{

class StepCache;
class SuccessCache;

struct RunOptions
{
    bool dryRun = false;
    bool enableCache = true;
    std::optional<std::size_t> maxThreads;
    logging::OutputMode outputMode = logging::OutputMode::Clean;
    logging::ILogger* logger = nullptr;
    StepCache* stepCache = nullptr;
    SuccessCache* successCache = nullptr;
    CacheOptions cache;
    PerformanceSettings performance;
    UiSettings ui;
    logging::LoggingSettings logging;
    logging::RunLogWriter* runLogWriter = nullptr;
};

}  // namespace beez::core
