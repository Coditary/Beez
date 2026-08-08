#include "beez/logging/spdlog_backend.hpp"

#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace beez::logging
{

namespace
{

class SpdlogLogger final : public ILogger
{
  public:
    explicit SpdlogLogger(OutputMode mode) : mode_(mode), logger_(spdlog::stdout_color_mt("beez"))
    {
        logger_->set_pattern("%v");
    }

    void beginRun(const std::string& runKind, const std::string& name) override
    {
        logger_->info("Starting {}: {}", runKind, name);
        logger_->info("============================================================");
    }

    void logProgress(const ExecutionProgress& progress) override
    {
        logger_->info(
            "[{}/{}] {} | {}", progress.index, progress.total, progress.category, progress.detail);
    }

    void logCommandOutput(LogChannelId channel, std::string_view output) override
    {
        if (mode_ != OutputMode::Verbose || output.empty())
        {
            return;
        }

        appendOutput(channel, output);
    }

    void logFailureOutput(std::string_view output) override
    {
        if (output.empty())
        {
            return;
        }

        appendOutput({}, output);
    }

    void endRun(bool success, double durationSeconds) override
    {
        flushChannelBuffers();
        logger_->info("============================================================");
        if (success)
        {
            logger_->info("Build successful in {:.2f}s!", durationSeconds);
            return;
        }

        logger_->info("Build failed after {:.2f}s.", durationSeconds);
    }

    LogChannelId openChannel(const std::string& label) override
    {
        const LogChannelId Channel {.value = nextChannelId_++};
        const std::scoped_lock Lock(mutex_);
        channelLabels_[Channel.value] = label;
        channelBuffers_[Channel.value] = {};
        return Channel;
    }

    void closeChannel(LogChannelId channel) override
    {
        const std::scoped_lock Lock(mutex_);
        flushChannelBuffer(channel.value);
        channelBuffers_.erase(channel.value);
        channelLabels_.erase(channel.value);
    }

  private:
    void appendOutput(LogChannelId channel, std::string_view output)
    {
        const std::scoped_lock Lock(mutex_);
        auto& buffer = channelBuffers_[channel.value];
        buffer.append(output.data(), output.size());

        std::size_t newlinePosition = 0;
        while ((newlinePosition = buffer.find('\n')) != std::string::npos)
        {
            const std::string Line = buffer.substr(0, newlinePosition);
            if (!Line.empty())
            {
                logger_->info("  | {}", Line);
            }
            buffer.erase(0, newlinePosition + 1);
        }
    }

    void flushChannelBuffers()
    {
        for (const auto& [channelId, buffer] : channelBuffers_)
        {
            if (!buffer.empty())
            {
                logger_->info("  | {}", buffer);
            }
            (void)channelId;
        }
        channelBuffers_.clear();
    }

    void flushChannelBuffer(std::uint64_t channelId)
    {
        const auto Found = channelBuffers_.find(channelId);
        if (Found == channelBuffers_.end() || Found->second.empty())
        {
            return;
        }

        logger_->info("  | {}", Found->second);
        Found->second.clear();
    }

    OutputMode mode_;
    std::shared_ptr<spdlog::logger> logger_;
    std::mutex mutex_;
    std::uint64_t nextChannelId_ = 1;
    std::unordered_map<std::uint64_t, std::string> channelLabels_;
    std::unordered_map<std::uint64_t, std::string> channelBuffers_;
};

}  // namespace

std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode)
{
    return std::make_unique<SpdlogLogger>(mode);
}

}  // namespace beez::logging
