#include "beez/core/execution/process/stream_capture.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

TEST(StreamCaptureTest, CapturesStdoutFromCallable)
{
    const auto Captured = beez::core::captureProcessOutput(
        []()
        {
            // NOLINTNEXTLINE(cert-err33-c,concurrency-mt-unsafe)
            static_cast<void>(std::fputs("captured-line\n", stdout));
            return 0;
        });

    EXPECT_EQ(Captured.exitCode, 0);
    EXPECT_NE(Captured.output.find("captured-line"), std::string::npos);
}

TEST(StreamCaptureTest, PropagatesExitCode)
{
    const auto Captured = beez::core::captureProcessOutput([]() { return 7; });
    EXPECT_EQ(Captured.exitCode, 7);
}

TEST(StreamCaptureTest, DiscardProcessOutputPropagatesExitCode)
{
    EXPECT_EQ(beez::core::discardProcessOutput([]() { return 9; }), 9);
}
