#include "beez/logging/logging_settings.hpp"
#include "beez/logging/run_log_writer.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>

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
    EXPECT_EQ(beez::logging::parseWorkerLogMode("always"), beez::logging::WorkerLogMode::Always);
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
