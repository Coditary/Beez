#include "beez/logging/console/progress_spinner.hpp"

#include "beez/core/config/ui_options.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

TEST(ProgressSpinnerTest, ReportsTerminalAvailability)
{
    static_cast<void>(beez::logging::isAnimationTerminalAvailable());
}

TEST(ProgressSpinnerTest, StartsAndStopsAnimator)
{
    beez::logging::ProgressSpinnerAnimator animator;
    beez::core::UiSettings uiSettings;
    uiSettings.colors = false;
    uiSettings.animation.progress.style = beez::core::ProgressDisplayStyle::Minimal;
    uiSettings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerDots;
    uiSettings.animation.progress.indicatorSpinIntervalMs = 10;

    const beez::logging::ExecutionProgress Progress {
        .index = 1,
        .total = 3,
        .category = "test",
        .detail = "step",
    };

    animator.start(uiSettings, Progress);
    EXPECT_TRUE(animator.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    animator.stop(true);
    EXPECT_FALSE(animator.isRunning());
}

TEST(ProgressSpinnerTest, StopWithoutFinalizeNewline)
{
    beez::logging::ProgressSpinnerAnimator animator;
    beez::core::UiSettings uiSettings;
    uiSettings.colors = false;
    uiSettings.animation.progress.indicatorSpinIntervalMs = 5;

    const beez::logging::ExecutionProgress Progress {
        .index = 2,
        .total = 4,
        .category = "qa",
        .detail = "lint",
    };

    animator.start(uiSettings, Progress);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    animator.stop(false);
    EXPECT_FALSE(animator.isRunning());
}
