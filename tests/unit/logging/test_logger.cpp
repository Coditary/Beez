#include "beez/logging/logger.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(RecordingLoggerTest, RecordsRunLifecycle)
{
    beez::logging::RecordingLogger logger;

    logger.beginRun("Workflow", "build");
    logger.logProgress({.index = 1, .total = 2, .category = "generate", .detail = "step: gen"});
    logger.logCommandOutput({}, "line one\n");
    logger.logFailureOutput("failure line\n");
    logger.endRun(true, 1.42);

    ASSERT_EQ(logger.lines().size(), 5U);
    EXPECT_EQ(logger.lines()[0].kind, beez::logging::RecordedLine::Kind::BeginRun);
    EXPECT_EQ(logger.lines()[0].text, "Workflow:build");
    EXPECT_EQ(logger.lines()[1].progress.detail, "step: gen");
    EXPECT_EQ(logger.lines()[2].text, "line one\n");
    EXPECT_EQ(logger.lines()[3].kind, beez::logging::RecordedLine::Kind::FailureOutput);
    EXPECT_EQ(logger.lines()[3].text, "failure line\n");
    EXPECT_TRUE(logger.lines()[4].success);
    EXPECT_DOUBLE_EQ(logger.lines()[4].durationSeconds, 1.42);
}

TEST(RecordingLoggerTest, TracksParallelChannels)
{
    beez::logging::RecordingLogger logger;

    const auto Channel = logger.openChannel("compile:code");
    logger.logCommandOutput(Channel, "compiling\n");
    logger.closeChannel(Channel);

    ASSERT_EQ(logger.lines().size(), 3U);
    EXPECT_EQ(logger.lines()[0].kind, beez::logging::RecordedLine::Kind::OpenChannel);
    EXPECT_EQ(logger.lines()[0].text, "compile:code");
    EXPECT_EQ(logger.lines()[1].kind, beez::logging::RecordedLine::Kind::CommandOutput);
    EXPECT_EQ(logger.lines()[2].kind, beez::logging::RecordedLine::Kind::CloseChannel);
}

TEST(NullLoggerTest, DoesNotThrow)
{
    beez::logging::NullLogger logger;
    logger.beginRun("Task", "noop");
    logger.logProgress({.index = 1, .total = 1, .category = "task", .detail = "task: noop"});
    logger.logCommandOutput({}, "ignored");
    logger.logFailureOutput("also ignored");
    logger.endRun(false, 0.0);
    const auto Channel = logger.openChannel("parallel");
    logger.closeChannel(Channel);
    SUCCEED();
}
