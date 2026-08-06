#pragma once

#include "beez/logging/output_mode.hpp"

namespace beez::logging
{
class ILogger;
}  // namespace beez::logging

namespace beez::core
{

class StepCache;

struct RunOptions
{
    bool dryRun = false;
    bool enableCache = true;
    logging::OutputMode outputMode = logging::OutputMode::Clean;
    logging::ILogger* logger = nullptr;
    StepCache* stepCache = nullptr;
};

}  // namespace beez::core
