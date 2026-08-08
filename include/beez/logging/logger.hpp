#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace beez::logging
{

struct ExecutionProgress
{
    std::size_t index = 0;
    std::size_t total = 0;
    std::string category;
    std::string detail;
};

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
    virtual void logFailureOutput(std::string_view output) = 0;
    virtual void endRun(bool success, double durationSeconds) = 0;

    // Future TUI: each channel represents a stack frame (workflow / phase / thread).
    virtual LogChannelId openChannel(const std::string& label) = 0;
    virtual void closeChannel(LogChannelId channel) = 0;
};

class NullLogger : public ILogger
{
  public:
    void beginRun(const std::string& /*runKind*/, const std::string& /*name*/) override {}
    void logProgress(const ExecutionProgress& /*progress*/) override {}
    void logCommandOutput(LogChannelId /*channel*/, std::string_view /*output*/) override {}
    void logFailureOutput(std::string_view /*output*/) override {}
    void endRun(bool /*success*/, double /*durationSeconds*/) override {}
    LogChannelId openChannel(const std::string& /*label*/) override
    {
        return {};
    }
    void closeChannel(LogChannelId /*channel*/) override {}
};

struct RecordedLine
{
    enum class Kind : std::uint8_t
    {
        BeginRun,
        Progress,
        CommandOutput,
        FailureOutput,
        EndRun,
        OpenChannel,
        CloseChannel,
    };

    Kind kind = Kind::BeginRun;
    std::string text;
    ExecutionProgress progress {};
    LogChannelId channel {};
    bool success = false;
    double durationSeconds = 0.0;
};

class RecordingLogger : public ILogger
{
  public:
    void beginRun(const std::string& runKind, const std::string& name) override;
    void logProgress(const ExecutionProgress& progress) override;
    void logCommandOutput(LogChannelId channel, std::string_view output) override;
    void logFailureOutput(std::string_view output) override;
    void endRun(bool success, double durationSeconds) override;
    LogChannelId openChannel(const std::string& label) override;
    void closeChannel(LogChannelId channel) override;

    [[nodiscard]] const std::vector<RecordedLine>& lines() const
    {
        return lines_;
    }

  private:
    std::vector<RecordedLine> lines_;
    std::uint64_t nextChannelId_ = 1;
};

}  // namespace beez::logging
