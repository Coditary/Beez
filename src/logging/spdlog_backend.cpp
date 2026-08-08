// NOLINTBEGIN(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-special-member-functions)
#include "beez/logging/spdlog_backend.hpp"

#include "beez/core/ui_options.hpp"
#include "beez/logging/logger.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/logging/progress_spinner.hpp"
#include "beez/logging/worker_output_format.hpp"

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <unistd.h>

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

[[nodiscard]] spdlog::level::level_enum toSpdlogLevel(const core::UiLogLevel level)
{
    switch (level)
    {
    case core::UiLogLevel::Warn:
        return spdlog::level::warn;
    case core::UiLogLevel::Error:
        return spdlog::level::err;
    case core::UiLogLevel::Info:
        break;
    }

    return spdlog::level::info;
}

class SpdlogLogger final : public ILogger
{
  public:
    SpdlogLogger(OutputMode mode, core::UiSettings uiSettings)
        : mode_(mode), ui_(std::move(uiSettings)), logger_(spdlog::stdout_color_mt("beez"))
    {
        logger_->set_pattern("%v");
        logger_->set_level(toSpdlogLevel(ui_.logLevel));
    }

    ~SpdlogLogger() override
    {
        progressSpinner_.stop(false);
    }

    void beginRun(const std::string& runKind, const std::string& name) override
    {
        logger_->info("Starting {}: {}", runKind, name);
        logger_->info("============================================================");
    }

    void logProgress(const ExecutionProgress& progress) override
    {
        progressSpinner_.stop();

        if (shouldAnimateProgress(progress))
        {
            progressSpinner_.start(ui_, progress);
            return;
        }

        logger_->info("{}", core::formatProgressLine(ui_, progress, progress.cached));
    }

    void logCommandOutput(LogChannelId channel, std::string_view output) override
    {
        if (mode_ != OutputMode::Verbose || output.empty())
        {
            return;
        }

        appendOutput(channel, output);
    }

    void logFailureOutput(std::string_view output, const LogChannelId channel = {}) override
    {
        progressSpinner_.stop();

        if (output.empty())
        {
            return;
        }

        appendOutput(channel, output);
    }

    void endRun(bool success, double durationSeconds, const RunSummary& summary) override
    {
        progressSpinner_.stop();
        flushChannelBuffers();

        const std::vector<std::string> Lines =
            core::formatRunEndMessage(ui_, success, durationSeconds, summary);
        for (const auto& line : Lines)
        {
            logger_->info("{}", line);
        }
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
    [[nodiscard]] bool shouldAnimateProgress(const ExecutionProgress& progress) const
    {
        return mode_ == OutputMode::Clean && isAnimationTerminalAvailable() && !progress.cached &&
               core::usesAnimatedProgressSpinner(ui_);
    }

    void logWorkerLine(LogChannelId channel, const std::string& line)
    {
        const std::string Prefix =
            core::formatWorkerOutputPrefix(ui_, channel.value, channelLabel(channel.value));
        const std::size_t TerminalWidth = isatty(STDOUT_FILENO) != 0 ? stdoutTerminalWidth() : 0;
        const std::size_t PrefixWidth = Prefix.size();
        const std::size_t ContentWidth =
            TerminalWidth > PrefixWidth ? TerminalWidth - PrefixWidth : 0;

        for (const std::string_view Segment : splitWorkerOutputLine(line, ContentWidth))
        {
            logger_->info("{}{}", Prefix, Segment);
        }
    }

    [[nodiscard]] std::string_view channelLabel(std::uint64_t channelId) const
    {
        const auto Found = channelLabels_.find(channelId);
        if (Found == channelLabels_.end())
        {
            return {};
        }

        return Found->second;
    }

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
                logWorkerLine(channel, Line);
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
                logWorkerLine(LogChannelId {.value = channelId}, buffer);
            }
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

        logWorkerLine(LogChannelId {.value = channelId}, Found->second);
        Found->second.clear();
    }

    OutputMode mode_;
    core::UiSettings ui_;
    std::shared_ptr<spdlog::logger> logger_;
    ProgressSpinnerAnimator progressSpinner_;
    std::mutex mutex_;
    std::uint64_t nextChannelId_ = 1;
    std::unordered_map<std::uint64_t, std::string> channelLabels_;
    std::unordered_map<std::uint64_t, std::string> channelBuffers_;
};

}  // namespace

std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode, const core::UiSettings& uiSettings)
{
    return std::make_unique<SpdlogLogger>(mode, uiSettings);
}

}  // namespace beez::logging
// NOLINTEND(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-special-member-functions)
