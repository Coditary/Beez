#include "beez/logging/output_mode.hpp"

namespace beez::logging
{

bool writesProgressToConsole(OutputMode mode)
{
    return mode == OutputMode::Clean || mode == OutputMode::Verbose;
}

bool writesFailureOutputToConsole(OutputMode mode)
{
    return mode != OutputMode::Silent;
}

bool writesCliErrorsToConsole(OutputMode mode)
{
    return mode != OutputMode::Silent;
}

bool writesRunSummaryToConsole(OutputMode mode, bool success)
{
    if (mode == OutputMode::Silent)
    {
        return false;
    }

    if (mode == OutputMode::Errors)
    {
        return !success;
    }

    return true;
}

const char* outputModeToString(OutputMode mode)
{
    switch (mode)
    {
    case OutputMode::Verbose:
        return "verbose";
    case OutputMode::Errors:
        return "errors";
    case OutputMode::Silent:
        return "silent";
    case OutputMode::Clean:
        break;
    }

    return "clean";
}

}  // namespace beez::logging
