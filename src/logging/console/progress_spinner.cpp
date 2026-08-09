// NOLINTBEGIN(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,cppcoreguidelines-pro-type-vararg)
#include "beez/logging/console/progress_spinner.hpp"

#include "beez/core/config/ui_options.hpp"
#include "beez/logging/console/worker_output_format.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace beez::logging
{

namespace
{

[[nodiscard]] std::string padProgressLine(std::string line, const std::size_t terminalWidth)
{
    if (terminalWidth == 0 || line.size() >= terminalWidth)
    {
        return line;
    }

    line.append(terminalWidth - line.size(), ' ');
    return line;
}

[[nodiscard]] std::size_t terminalWidthForFd(const int fd, const std::size_t fallback)
{
    winsize size {};
    if (fd >= 0 && ioctl(fd, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
    {
        return size.ws_col;
    }

    return fallback;
}

void writeToFd(const int fd, const char* data, const std::size_t size)
{
    if (fd < 0 || size == 0)
    {
        return;
    }

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    static_cast<void>(write(fd, data, size));
}

}  // namespace

bool isAnimationTerminalAvailable()
{
    if (isatty(STDOUT_FILENO) != 0)
    {
        return true;
    }

    const int TtyFd = ::open("/dev/tty", O_WRONLY);
    if (TtyFd >= 0)
    {
        ::close(TtyFd);
        return true;
    }

    return false;
}

ProgressSpinnerAnimator::~ProgressSpinnerAnimator()
{
    stop(false);
}

void ProgressSpinnerAnimator::openOutputFd()
{
    closeOutputFd();
    outputFd_ = ::open("/dev/tty", O_WRONLY);
    if (outputFd_ < 0)
    {
        outputFd_ = dup(STDOUT_FILENO);
    }

    terminalWidth_ = terminalWidthForFd(outputFd_, stdoutTerminalWidth());
}

void ProgressSpinnerAnimator::closeOutputFd()
{
    if (outputFd_ >= 0)
    {
        ::close(outputFd_);
        outputFd_ = -1;
    }
}

void ProgressSpinnerAnimator::start(const core::UiSettings& uiSettings,
                                    const ExecutionProgress& progress)
{
    stop(false);

    {
        const std::scoped_lock Lock(mutex_);
        uiSettings_ = uiSettings;
        progress_ = progress;
        interval_ =
            std::chrono::milliseconds(uiSettings.animation.progress.indicatorSpinIntervalMs);
        stopRequested_.store(false);
        running_.store(true);
        openOutputFd();
    }

    writeFrame(0);
    std::thread worker([this]() { run(); });
    {
        const std::scoped_lock Lock(mutex_);
        thread_ = std::move(worker);
    }
}

void ProgressSpinnerAnimator::stop(const bool finalizeNewline)
{
    stopRequested_.store(true);

    bool wasAnimating = false;
    int outputFd = -1;
    std::thread worker;
    {
        const std::scoped_lock Lock(mutex_);
        if (thread_.joinable())
        {
            wasAnimating = true;
            worker = std::move(thread_);
        }
        outputFd = outputFd_;
    }

    if (worker.joinable())
    {
        worker.join();
    }

    running_.store(false);
    stopRequested_.store(false);

    if (finalizeNewline && wasAnimating)
    {
        writeToFd(outputFd, "\n", 1);
    }

    {
        const std::scoped_lock Lock(mutex_);
        closeOutputFd();
    }
}

void ProgressSpinnerAnimator::run()
{
    std::size_t frameIndex = 1;
    while (!stopRequested_.load())
    {
        writeFrame(frameIndex);
        ++frameIndex;

        const auto SleepUntil = std::chrono::steady_clock::now() + interval_;
        while (!stopRequested_.load() && std::chrono::steady_clock::now() < SleepUntil)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void ProgressSpinnerAnimator::writeFrame(const std::size_t frameIndex)
{
    core::UiSettings uiSettings;
    ExecutionProgress progress;
    int outputFd = -1;
    std::size_t terminalWidth = 0;
    {
        const std::scoped_lock Lock(mutex_);
        uiSettings = uiSettings_;
        progress = progress_;
        outputFd = outputFd_;
        terminalWidth = terminalWidth_;
    }

    const std::string Line =
        core::formatProgressLine(uiSettings, progress, progress.cached, frameIndex);
    const std::string PaddedLine = padProgressLine(Line, terminalWidth);

    writeToFd(outputFd, "\r", 1);
    writeToFd(outputFd, PaddedLine.data(), PaddedLine.size());
}

}  // namespace beez::logging
// NOLINTEND(misc-include-cleaner,readability-identifier-length,readability-identifier-naming,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,cppcoreguidelines-pro-type-vararg)
