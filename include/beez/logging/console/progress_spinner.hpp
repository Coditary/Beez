#pragma once

#include "beez/core/config/ui_options.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>

namespace beez::logging
{

[[nodiscard]] bool isAnimationTerminalAvailable();

class ProgressSpinnerAnimator
{
  public:
    ProgressSpinnerAnimator() = default;
    ~ProgressSpinnerAnimator();

    ProgressSpinnerAnimator(const ProgressSpinnerAnimator&) = delete;
    ProgressSpinnerAnimator& operator=(const ProgressSpinnerAnimator&) = delete;
    ProgressSpinnerAnimator(ProgressSpinnerAnimator&&) = delete;
    ProgressSpinnerAnimator& operator=(ProgressSpinnerAnimator&&) = delete;

    void start(const core::UiSettings& uiSettings, const ExecutionProgress& progress);
    void stop(bool finalizeNewline = true);

    [[nodiscard]] bool isRunning() const
    {
        return running_.load();
    }

  private:
    void openOutputFd();
    void closeOutputFd();
    void run();
    void writeFrame(std::size_t frameIndex);

    std::mutex mutex_;
    std::thread thread_;
    std::atomic<bool> stopRequested_ {true};
    std::atomic<bool> running_ {false};
    core::UiSettings uiSettings_ {};
    ExecutionProgress progress_ {};
    std::chrono::milliseconds interval_ {core::DefaultIndicatorSpinIntervalMs};
    std::size_t terminalWidth_ = 0;
    int outputFd_ = -1;
};

}  // namespace beez::logging
