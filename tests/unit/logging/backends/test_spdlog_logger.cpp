#include "beez/core/config/ui/types.hpp"
#include "beez/logging/backends/spdlog_logger.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <filesystem>
#include <gtest/gtest.h>

TEST(SpdlogLoggerTest, CreatesLoggerForSilentMode)
{
    const beez::core::UiSettings UiSettings;
    const beez::logging::LoggingSettings LoggingSettings {.runLog = false};

    const auto Logger = beez::logging::createSpdlogLogger(
        beez::logging::OutputMode::Silent, UiSettings, LoggingSettings);
    ASSERT_NE(Logger, nullptr);

    Logger->beginRun("Task", "noop");
    Logger->logProgress({.index = 1, .total = 1, .category = "task", .detail = "task: noop"});
    Logger->endRun(true, 0.0);
}

TEST(SpdlogLoggerTest, CreatesLoggerWithRunLogFile)
{
    beez::logging::LoggingSettings settings;
    settings.runLog = true;
    settings.runLogFile = std::filesystem::temp_directory_path() / "beez_spdlog_test.log";

    const auto Logger = beez::logging::createSpdlogLogger(
        beez::logging::OutputMode::Silent, beez::core::UiSettings {}, settings);
    ASSERT_NE(Logger, nullptr);
    Logger->beginRun("Workflow", "build");
    Logger->endRun(true, 1.0);

    EXPECT_TRUE(std::filesystem::exists(settings.runLogFile));
    std::filesystem::remove(settings.runLogFile);
}
