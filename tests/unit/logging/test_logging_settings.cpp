#include "beez/logging/logging_settings.hpp"
#include "beez/logging/run_log_writer.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
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

TEST(RunLogWriterTest, WritesWorkerOutputOnFailure)
{
    const std::filesystem::path TempRoot =
        std::filesystem::temp_directory_path() / "beez_run_log_writer_test";
    std::filesystem::remove_all(TempRoot);

    beez::logging::LoggingSettings settings;
    settings.workers = beez::logging::WorkerLogMode::OnFailure;
    settings.workersDir = TempRoot / "workers";

    beez::logging::RunLogWriter writer(settings);
    writer.writeWorkerOutput("qa:lint", "tidy_1", "lint failed\n", 1);

    const auto WorkerLog = settings.workersDir / "qa_lint" / "tidy_1.log";
    ASSERT_TRUE(std::filesystem::exists(WorkerLog));

    std::ifstream stream(WorkerLog);
    const std::string Contents((std::istreambuf_iterator<char>(stream)),
                               std::istreambuf_iterator<char>());
    EXPECT_NE(Contents.find("lint failed"), std::string::npos);
    EXPECT_NE(Contents.find("# exit: 1"), std::string::npos);

    std::filesystem::remove_all(TempRoot);
}

TEST(RunLogWriterTest, SkipsSuccessfulWorkersInOnFailureMode)
{
    const std::filesystem::path TempRoot =
        std::filesystem::temp_directory_path() / "beez_run_log_writer_skip_test";
    std::filesystem::remove_all(TempRoot);

    beez::logging::LoggingSettings settings;
    settings.workers = beez::logging::WorkerLogMode::OnFailure;
    settings.workersDir = TempRoot / "workers";

    beez::logging::RunLogWriter writer(settings);
    writer.writeWorkerOutput("qa:lint", "tidy_1", "all good\n", 0);

    EXPECT_FALSE(std::filesystem::exists(settings.workersDir / "qa_lint" / "tidy_1.log"));
    std::filesystem::remove_all(TempRoot);
}
