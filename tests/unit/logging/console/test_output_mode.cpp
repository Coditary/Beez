#include "beez/logging/console/output_mode.hpp"

#include <gtest/gtest.h>

TEST(OutputModeTest, ConsoleHelpersMatchModes)
{
    using beez::logging::OutputMode;

    EXPECT_TRUE(beez::logging::writesProgressToConsole(OutputMode::Clean));
    EXPECT_TRUE(beez::logging::writesProgressToConsole(OutputMode::Verbose));
    EXPECT_FALSE(beez::logging::writesProgressToConsole(OutputMode::Errors));
    EXPECT_FALSE(beez::logging::writesProgressToConsole(OutputMode::Silent));

    EXPECT_TRUE(beez::logging::writesFailureOutputToConsole(OutputMode::Clean));
    EXPECT_TRUE(beez::logging::writesFailureOutputToConsole(OutputMode::Errors));
    EXPECT_FALSE(beez::logging::writesFailureOutputToConsole(OutputMode::Silent));

    EXPECT_TRUE(beez::logging::writesRunSummaryToConsole(OutputMode::Clean, true));
    EXPECT_FALSE(beez::logging::writesRunSummaryToConsole(OutputMode::Errors, true));
    EXPECT_TRUE(beez::logging::writesRunSummaryToConsole(OutputMode::Errors, false));
    EXPECT_FALSE(beez::logging::writesRunSummaryToConsole(OutputMode::Silent, false));
}

TEST(OutputModeTest, ToStringLabels)
{
    using beez::logging::OutputMode;

    EXPECT_STREQ(beez::logging::outputModeToString(OutputMode::Clean), "clean");
    EXPECT_STREQ(beez::logging::outputModeToString(OutputMode::Verbose), "verbose");
    EXPECT_STREQ(beez::logging::outputModeToString(OutputMode::Errors), "errors");
    EXPECT_STREQ(beez::logging::outputModeToString(OutputMode::Silent), "silent");
}
