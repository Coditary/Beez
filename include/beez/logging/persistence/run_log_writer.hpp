#pragma once

#include "beez/logging/settings/logging_settings.hpp"

#include <mutex>
#include <string>
#include <string_view>

namespace beez::logging
{

class RunLogWriter
{
  public:
    explicit RunLogWriter(LoggingSettings settings);

    [[nodiscard]] const LoggingSettings& settings() const
    {
        return settings_;
    }

    [[nodiscard]] bool shouldCaptureWorkerOutput() const;

    [[nodiscard]] bool shouldPersistWorkerOutput(int exitCode) const;

    void writeWorkerOutput(std::string_view stepName,
                           std::string_view workerName,
                           std::string_view output,
                           int exitCode);

  private:
    [[nodiscard]] static std::string sanitizeLogComponent(std::string_view value);

    LoggingSettings settings_;
    std::mutex mutex_;
};

}  // namespace beez::logging
