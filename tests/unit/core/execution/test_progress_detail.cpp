#include "beez/core/execution/progress_detail.hpp"

#include "beez/core/model/step.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(ProgressDetailTest, TruncatesLongCommands)
{
    const std::string LongCommand(70, 'a');
    const std::string Truncated = beez::core::truncateForDisplay(LongCommand);
    EXPECT_EQ(Truncated.size(), beez::core::DefaultProgressDetailLength);
    EXPECT_EQ(Truncated.substr(beez::core::DefaultProgressDetailLength - 3), "...");
}

TEST(ProgressDetailTest, NormalizesNewlines)
{
    EXPECT_EQ(beez::core::truncateForDisplay("echo\nhello"), "echo hello");
}

TEST(ProgressDetailTest, StepDetailPrefersDescription)
{
    beez::core::Step step;
    step.name = "compile:lua";
    step.description = "Compile Lua shaders";

    EXPECT_EQ(beez::core::stepProgressDetail(step), "Compile Lua shaders");
}

TEST(ProgressDetailTest, StepDetailFallsBackToName)
{
    beez::core::Step step;
    step.name = "generate:code";

    EXPECT_EQ(beez::core::stepProgressDetail(step), "generate:code");
}
