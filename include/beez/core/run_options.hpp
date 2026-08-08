#pragma once

#include "beez/core/cache_options.hpp"
#include "beez/logging/output_mode.hpp"

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
};

}  // namespace beez::core
