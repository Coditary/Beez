#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace beez::logging
{

enum class WorkerLogMode : std::uint8_t
{
    Off,
    OnFailure,
    Always,
};

struct LoggingSettingsOverlay
{
    std::optional<bool> runLog;
    std::optional<std::filesystem::path> runLogFile;
    std::optional<bool> logSteps;
    std::optional<std::string> workers;
    std::optional<std::filesystem::path> workersDir;
};

struct LoggingSettings
{
    bool runLog = true;
    std::filesystem::path runLogFile;
    bool logSteps = false;
    WorkerLogMode workers = WorkerLogMode::OnFailure;
    std::filesystem::path workersDir;
};

[[nodiscard]] WorkerLogMode parseWorkerLogMode(const std::string& value);

[[nodiscard]] const char* toString(WorkerLogMode mode);

[[nodiscard]] std::vector<const char*> workerLogModeNames();

[[nodiscard]] LoggingSettings resolveLoggingSettings(const LoggingSettingsOverlay& overlay,
                                                     const std::filesystem::path& projectRoot);

void mergeLoggingSettingsOverlay(LoggingSettingsOverlay& target,
                                 const LoggingSettingsOverlay& overlay);

[[nodiscard]] std::filesystem::path resolveLoggingPath(const std::filesystem::path& path,
                                                       const std::filesystem::path& projectRoot);

}  // namespace beez::logging
