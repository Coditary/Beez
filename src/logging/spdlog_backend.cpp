// NOLINTBEGIN(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-special-member-functions)
#include "beez/logging/spdlog_backend.hpp"

#include "beez/core/ui_options.hpp"
#include "beez/logging/logger.hpp"
#include "beez/logging/logging_settings.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/logging/progress_spinner.hpp"
#include "beez/logging/worker_output_format.hpp"

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
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

[[nodiscard]] std::shared_ptr<spdlog::logger> makeConsoleLogger(const core::UiSettings& uiSettings)
{
    auto logger = spdlog::stdout_color_mt("beez");
    logger->set_pattern("%v");
    logger->set_level(toSpdlogLevel(uiSettings.logLevel));
    return logger;
}

[[nodiscard]] std::shared_ptr<spdlog::logger> makeFileLogger(const std::filesystem::path& logFile,
                                                             const core::UiSettings& uiSettings)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logFile.parent_path(), errorCode);

    auto logger = spdlog::basic_logger_mt("beez_file", logFile.string(), true);
    logger->set_pattern("%v");
    logger->set_level(toSpdlogLevel(uiSettings.logLevel));
    return logger;
}

class SpdlogLogger final : public ILogger
{
  public:
    SpdlogLogger(OutputMode mode, core::UiSettings uiSettings, LoggingSettings loggingSettings)
        : mode_(mode), ui_(std::move(uiSettings)), loggingSettings_(std::move(loggingSettings)),
          consoleLogger_(makeConsoleLogger(ui_))
    {
        if (loggingSettings_.runLog)
        {
            fileLogger_ = makeFileLogger(loggingSettings_.runLogFile, ui_);
        }
    }

    ~SpdlogLogger() override
    {
        progressSpinner_.stop(false);
    }

    void beginRun(const std::string& runKind, const std::string& name) override
    {
        writeRunLine("Starting " + runKind + ": " + name);
        writeRunLine("============================================================");
    }

    void logProgress(const ExecutionProgress& progress) override
    {
        progressSpinner_.stop();

        if (shouldAnimateProgress(progress))
        {
            progressSpinner_.start(ui_, progress);
            if (loggingSettings_.logSteps)
            {
                writeFileLine(core::formatProgressLine(ui_, progress, progress.cached));
            }
            return;
        }

        const std::string Line = core::formatProgressLine(ui_, progress, progress.cached);
        consoleLogger_->info("{}", Line);
        if (loggingSettings_.logSteps)
        {
            writeFileLine(Line);
        }
    }

    void logCommandOutput(LogChannelId channel, std::string_view output) override
    {
        if (mode_ != OutputMode::Verbose || output.empty())
        {
            return;
        }

        appendOutput(channel, output, true, true);
    }

    void logFailureOutput(std::string_view output, const LogChannelId channel = {}) override
    {
        progressSpinner_.stop();

        if (output.empty())
        {
            return;
        }

        appendOutput(channel, output, true, true);
    }

    void endRun(bool success, double durationSeconds, const RunSummary& summary) override
    {
        progressSpinner_.stop();
        flushChannelBuffers(true, true);

        const std::vector<std::string> Lines =
            core::formatRunEndMessage(ui_, success, durationSeconds, summary);
        for (const auto& line : Lines)
        {
            writeRunLine(line);
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
        flushChannelBuffer(channel.value, true, true);
        channelBuffers_.erase(channel.value);
        channelLabels_.erase(channel.value);
    }

  private:
    void writeRunLine(const std::string& line)
    {
        consoleLogger_->info("{}", line);
        if (fileLogger_ != nullptr)
        {
            fileLogger_->info("{}", line);
        }
    }

    void writeFileLine(const std::string& line)
    {
        if (fileLogger_ != nullptr)
        {
            fileLogger_->info("{}", line);
        }
    }

    [[nodiscard]] bool shouldAnimateProgress(const ExecutionProgress& progress) const
    {
        return mode_ == OutputMode::Clean && isAnimationTerminalAvailable() && !progress.cached &&
               core::usesAnimatedProgressSpinner(ui_);
    }

    void logWorkerLine(LogChannelId channel, const std::string& line, bool toConsole, bool toFile)
    {
        const std::string Prefix =
            core::formatWorkerOutputPrefix(ui_, channel.value, channelLabel(channel.value));
        const std::size_t TerminalWidth = isatty(STDOUT_FILENO) != 0 ? stdoutTerminalWidth() : 0;
        const std::size_t PrefixWidth = Prefix.size();
        const std::size_t ContentWidth =
            TerminalWidth > PrefixWidth ? TerminalWidth - PrefixWidth : 0;

        for (const std::string_view Segment : splitWorkerOutputLine(line, ContentWidth))
        {
            if (toConsole)
            {
                consoleLogger_->info("{}{}", Prefix, Segment);
            }
            if (toFile && fileLogger_ != nullptr)
            {
                fileLogger_->info("{}{}", Prefix, Segment);
            }
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

    void appendOutput(LogChannelId channel, std::string_view output, bool toConsole, bool toFile)
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
                logWorkerLine(channel, Line, toConsole, toFile);
            }
            buffer.erase(0, newlinePosition + 1);
        }
    }

    void flushChannelBuffers(bool toConsole, bool toFile)
    {
        for (const auto& [channelId, buffer] : channelBuffers_)
        {
            if (!buffer.empty())
            {
                logWorkerLine(LogChannelId {.value = channelId}, buffer, toConsole, toFile);
            }
        }
        channelBuffers_.clear();
    }

    void flushChannelBuffer(std::uint64_t channelId, bool toConsole, bool toFile)
    {
        const auto Found = channelBuffers_.find(channelId);
        if (Found == channelBuffers_.end() || Found->second.empty())
        {
            return;
        }

        logWorkerLine(LogChannelId {.value = channelId}, Found->second, toConsole, toFile);
        Found->second.clear();
    }

    OutputMode mode_;
    core::UiSettings ui_;
    LoggingSettings loggingSettings_;
    std::shared_ptr<spdlog::logger> consoleLogger_;
    std::shared_ptr<spdlog::logger> fileLogger_;
    ProgressSpinnerAnimator progressSpinner_;
    std::mutex mutex_;
    std::uint64_t nextChannelId_ = 1;
    std::unordered_map<std::uint64_t, std::string> channelLabels_;
    std::unordered_map<std::uint64_t, std::string> channelBuffers_;
};

}  // namespace

std::unique_ptr<ILogger> createSpdlogLogger(OutputMode mode,
                                            const core::UiSettings& uiSettings,
                                            const LoggingSettings& loggingSettings)
{
    return std::make_unique<SpdlogLogger>(mode, uiSettings, loggingSettings);
}

}  // namespace beez::logging
// NOLINTEND(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-special-member-functions)
