#include "beez/logging/run_log_writer.hpp"

#include "beez/logging/logging_settings.hpp"

#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner)
#include <cctype>
#include <filesystem>
#include <ios>
// NOLINTEND(misc-include-cleaner)

namespace beez::logging
{

RunLogWriter::RunLogWriter(LoggingSettings settings) : settings_(std::move(settings)) {}

bool RunLogWriter::shouldCaptureWorkerOutput() const
{
    return settings_.workers != WorkerLogMode::Off;
}

bool RunLogWriter::shouldPersistWorkerOutput(const int ExitCode) const
{
    if (settings_.workers == WorkerLogMode::Off)
    {
        return false;
    }

    if (settings_.workers == WorkerLogMode::Always)
    {
        return true;
    }

    return ExitCode != 0;
}

std::string RunLogWriter::sanitizeLogComponent(const std::string_view Value)
{
    std::string sanitized;
    sanitized.reserve(Value.size());
    for (const char Character : Value)
    {
        if (std::isalnum(static_cast<unsigned char>(Character)) != 0 || Character == '-' ||
            Character == '_' || Character == '.')
        {
            sanitized.push_back(Character);
        }
        else
        {
            sanitized.push_back('_');
        }
    }

    if (sanitized.empty())
    {
        return "unnamed";
    }

    return sanitized;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void RunLogWriter::writeWorkerOutput(const std::string_view StepName,
                                     const std::string_view WorkerName,
                                     const std::string_view Output,
                                     const int ExitCode)
{
    if (!shouldPersistWorkerOutput(ExitCode) || Output.empty())
    {
        return;
    }

    const std::filesystem::path WorkerDirectory =
        settings_.workersDir / sanitizeLogComponent(StepName);
    const std::filesystem::path WorkerLogPath =
        WorkerDirectory / (sanitizeLogComponent(WorkerName) + ".log");

    const std::scoped_lock Lock(mutex_);
    std::error_code errorCode;
    std::filesystem::create_directories(WorkerDirectory, errorCode);

    std::ofstream stream(WorkerLogPath, std::ios::out | std::ios::trunc);
    if (!stream.is_open())
    {
        throw std::runtime_error("failed to open worker log file: " + WorkerLogPath.string());
    }

    stream << "# step: " << StepName << '\n';
    stream << "# worker: " << WorkerName << '\n';
    stream << "# exit: " << ExitCode << '\n';
    stream << Output;
    if (Output.back() != '\n')
    {
        stream << '\n';
    }
}
// NOLINTEND(bugprone-easily-swappable-parameters)

}  // namespace beez::logging
