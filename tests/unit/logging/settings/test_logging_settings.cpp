#include "beez/logging/settings/logging_settings.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

TEST(LoggingSettingsTest, ResolvesDefaultPathsRelativeToProjectRoot)
{
    const auto Settings = beez::logging::resolveLoggingSettings(
        beez::logging::LoggingSettingsOverlay {}, "/tmp/project");

    EXPECT_TRUE(Settings.runLog);
    EXPECT_FALSE(Settings.logSteps);
    EXPECT_EQ(Settings.workers, beez::logging::WorkerLogMode::OnFailure);
    EXPECT_EQ(Settings.runLogFile, std::filesystem::path("/tmp/project/.cache/logs/latest.log"));
    EXPECT_EQ(Settings.workersDir, std::filesystem::path("/tmp/project/.cache/logs/workers"));
}

TEST(LoggingSettingsTest, ParsesWorkerLogMode)
{
    EXPECT_EQ(beez::logging::parseWorkerLogMode("off"), beez::logging::WorkerLogMode::Off);
    EXPECT_EQ(beez::logging::parseWorkerLogMode("on_failure"),
              beez::logging::WorkerLogMode::OnFailure);
    EXPECT_EQ(beez::logging::parseWorkerLogMode("always"), beez::logging::WorkerLogMode::Always);
}

TEST(LoggingSettingsTest, RejectsInvalidWorkerLogMode)
{
    EXPECT_THROW(beez::logging::parseWorkerLogMode("invalid"), std::runtime_error);
}

TEST(LoggingSettingsTest, ToStringMapsWorkerLogModes)
{
    EXPECT_STREQ(beez::logging::toString(beez::logging::WorkerLogMode::Off), "off");
    EXPECT_STREQ(beez::logging::toString(beez::logging::WorkerLogMode::OnFailure), "on_failure");
    EXPECT_STREQ(beez::logging::toString(beez::logging::WorkerLogMode::Always), "always");
}

TEST(LoggingSettingsTest, WorkerLogModeNamesListsAll)
{
    const auto Names = beez::logging::workerLogModeNames();
    EXPECT_EQ(Names.size(), 3U);
}

TEST(LoggingSettingsTest, ResolvesAbsoluteLoggingPaths)
{
    beez::logging::LoggingSettingsOverlay overlay;
    overlay.runLogFile = std::filesystem::path("/var/log/beez.log");
    overlay.workersDir = std::filesystem::path("/var/log/workers");

    const auto Settings = beez::logging::resolveLoggingSettings(overlay, "/tmp/project");
    EXPECT_EQ(Settings.runLogFile, std::filesystem::path("/var/log/beez.log"));
    EXPECT_EQ(Settings.workersDir, std::filesystem::path("/var/log/workers"));
}

TEST(LoggingSettingsTest, MergeLoggingSettingsOverlay)
{
    beez::logging::LoggingSettingsOverlay target;
    beez::logging::LoggingSettingsOverlay overlay;
    overlay.runLog = false;
    overlay.logSteps = true;
    overlay.workers = "always";
    overlay.runLogFile = std::filesystem::path("logs/custom.log");
    overlay.workersDir = std::filesystem::path("logs/custom_workers");

    beez::logging::mergeLoggingSettingsOverlay(target, overlay);

    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- fields set in overlay above
    ASSERT_TRUE(target.runLog.has_value());
    EXPECT_FALSE(*target.runLog);
    ASSERT_TRUE(target.logSteps.has_value());
    EXPECT_TRUE(*target.logSteps);
    ASSERT_TRUE(target.workers.has_value());
    EXPECT_EQ(*target.workers, "always");
    ASSERT_TRUE(target.runLogFile.has_value());
    EXPECT_EQ(*target.runLogFile, std::filesystem::path("logs/custom.log"));
    ASSERT_TRUE(target.workersDir.has_value());
    EXPECT_EQ(*target.workersDir, std::filesystem::path("logs/custom_workers"));
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(LoggingSettingsTest, ResolvesCustomRunLogFileRelativeToProjectRoot)
{
    beez::logging::LoggingSettingsOverlay overlay;
    overlay.runLogFile = std::filesystem::path("custom/run.log");

    const auto Settings = beez::logging::resolveLoggingSettings(overlay, "/tmp/project");
    EXPECT_EQ(Settings.runLogFile, std::filesystem::path("/tmp/project/custom/run.log"));
}

TEST(LoggingSettingsTest, ResolvesCustomWorkersDirRelativeToProjectRoot)
{
    beez::logging::LoggingSettingsOverlay overlay;
    overlay.workersDir = std::filesystem::path("custom/workers");

    const auto Settings = beez::logging::resolveLoggingSettings(overlay, "/tmp/project");
    EXPECT_EQ(Settings.workersDir, std::filesystem::path("/tmp/project/custom/workers"));
}

TEST(LoggingSettingsTest, ResolvesLogStepsOverlay)
{
    beez::logging::LoggingSettingsOverlay overlay;
    overlay.logSteps = true;

    const auto Settings = beez::logging::resolveLoggingSettings(overlay, "/tmp/project");
    EXPECT_TRUE(Settings.logSteps);
}
