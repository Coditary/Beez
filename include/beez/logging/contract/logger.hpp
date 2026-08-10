#pragma once

#include "beez/logging/contract/run_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace beez::logging
{

// Identifies a scoped output channel for parallel execution contexts.
struct LogChannelId
{
    std::uint64_t value = 0;
};

class ILogger
{
  public:
    virtual ~ILogger() = default;
    ILogger() = default;
    ILogger(const ILogger&) = delete;
    ILogger& operator=(const ILogger&) = delete;
    ILogger(ILogger&&) = delete;
    ILogger& operator=(ILogger&&) = delete;

    virtual void beginRun(const std::string& runKind, const std::string& name) = 0;
    virtual void logProgress(const ExecutionProgress& progress) = 0;
    virtual void logCommandOutput(LogChannelId channel, std::string_view output) = 0;
    virtual void logFailureOutput(std::string_view output, LogChannelId channel = {}) = 0;
    virtual void endRun(bool success, double durationSeconds, const RunSummary& summary = {}) = 0;

    // Future TUI: each channel represents a stack frame (workflow / phase / thread).
    virtual LogChannelId openChannel(const std::string& label) = 0;
    virtual void closeChannel(LogChannelId channel) = 0;
};

}  // namespace beez::logging
