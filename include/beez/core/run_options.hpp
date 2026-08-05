#pragma once

#include "beez/logging/output_mode.hpp"

namespace beez::logging
{
class ILogger;
}  // namespace beez::logging

namespace beez::core
{

struct RunOptions
{
    bool dryRun = false;
    logging::OutputMode outputMode = logging::OutputMode::Clean;
    logging::ILogger* logger = nullptr;
};

}  // namespace beez::core
