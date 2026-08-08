#include "beez/logging/logging_settings.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace beez::logging
{

namespace
{

constexpr std::string_view DefaultRunLogFile = ".cache/logs/latest.log";
constexpr std::string_view DefaultWorkersLogDir = ".cache/logs/workers";

}  // namespace

WorkerLogMode parseWorkerLogMode(const std::string& value)
{
    if (value == "off")
    {
        return WorkerLogMode::Off;
    }
    if (value == "on_failure")
    {
        return WorkerLogMode::OnFailure;
    }
    if (value == "always")
    {
        return WorkerLogMode::Always;
    }

    throw std::runtime_error("ui.logging.workers must be 'off', 'on_failure', or 'always'");
}

const char* toString(const WorkerLogMode Mode)
{
    switch (Mode)
    {
    case WorkerLogMode::Off:
        return "off";
    case WorkerLogMode::OnFailure:
        return "on_failure";
    case WorkerLogMode::Always:
        return "always";
    }

    return "off";
}

std::vector<const char*> workerLogModeNames()
{
    return {"off", "on_failure", "always"};
}

std::filesystem::path resolveLoggingPath(const std::filesystem::path& path,
                                         const std::filesystem::path& projectRoot)
{
    if (path.empty())
    {
        return projectRoot / DefaultRunLogFile;
    }

    if (path.is_absolute())
    {
        return path;
    }

    return projectRoot / path;
}

LoggingSettings resolveLoggingSettings(const LoggingSettingsOverlay& overlay,
                                       const std::filesystem::path& projectRoot)
{
    LoggingSettings settings;
    settings.runLog = overlay.runLog.value_or(true);
    settings.runLogFile = resolveLoggingPath(
        overlay.runLogFile.value_or(std::filesystem::path(DefaultRunLogFile)), projectRoot);
    settings.logSteps = overlay.logSteps.value_or(false);
    settings.workers = overlay.workers.has_value() ? parseWorkerLogMode(*overlay.workers)
                                                   : WorkerLogMode::OnFailure;
    settings.workersDir = resolveLoggingPath(
        overlay.workersDir.value_or(std::filesystem::path(DefaultWorkersLogDir)), projectRoot);
    return settings;
}

void mergeLoggingSettingsOverlay(LoggingSettingsOverlay& target,
                                 const LoggingSettingsOverlay& overlay)
{
    if (overlay.runLog.has_value())
    {
        target.runLog = overlay.runLog;
    }
    if (overlay.runLogFile.has_value())
    {
        target.runLogFile = overlay.runLogFile;
    }
    if (overlay.logSteps.has_value())
    {
        target.logSteps = overlay.logSteps;
    }
    if (overlay.workers.has_value())
    {
        target.workers = overlay.workers;
    }
    if (overlay.workersDir.has_value())
    {
        target.workersDir = overlay.workersDir;
    }
}

}  // namespace beez::logging
