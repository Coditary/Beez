#include "beez/logging/logger.hpp"

#include <string>
#include <string_view>

namespace beez::logging
{

void RecordingLogger::beginRun(const std::string& runKind, const std::string& name)
{
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::BeginRun,
        .text = runKind + ":" + name,
    });
}

void RecordingLogger::logProgress(const ExecutionProgress& progress)
{
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::Progress,
        .progress = progress,
    });
}

void RecordingLogger::logCommandOutput(LogChannelId channel, std::string_view output)
{
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::CommandOutput,
        .text = std::string(output),
        .channel = channel,
    });
}

void RecordingLogger::endRun(bool success, double durationSeconds)
{
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::EndRun,
        .success = success,
        .durationSeconds = durationSeconds,
    });
}

LogChannelId RecordingLogger::openChannel(const std::string& label)
{
    const LogChannelId Channel {.value = nextChannelId_++};
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::OpenChannel,
        .text = label,
        .channel = Channel,
    });
    return Channel;
}

void RecordingLogger::closeChannel(LogChannelId channel)
{
    lines_.push_back(RecordedLine {
        .kind = RecordedLine::Kind::CloseChannel,
        .channel = channel,
    });
}

}  // namespace beez::logging
