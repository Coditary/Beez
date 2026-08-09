#pragma once

#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace beez::logging
{

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
    void logFailureOutput(std::string_view output, LogChannelId channel = {}) override;
    void endRun(bool success, double durationSeconds, const RunSummary& summary = {}) override;
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
