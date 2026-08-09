#include "beez/core/context.h"
#include "beez/plugin/shell/shell_executor.hpp"

#include <gtest/gtest.h>

TEST(ShellExecutorTest, TrueCommandReturnsZero)
{
    const beez::core::Context Ctx;
    beez::plugin::shell::ShellExecutor executor;

    EXPECT_EQ(executor.execute("true", Ctx), 0);
}

TEST(ShellExecutorTest, FalseCommandReturnsOne)
{
    const beez::core::Context Ctx;
    beez::plugin::shell::ShellExecutor executor;

    EXPECT_EQ(executor.execute("false", Ctx), 1);
}

TEST(ShellExecutorTest, CapturesStderrWhenRequested)
{
    const beez::core::Context Ctx;
    beez::plugin::shell::ShellExecutor executor;

    std::string capturedOutput;
    EXPECT_EQ(executor.execute("echo stderr-output >&2; exit 1", Ctx, &capturedOutput), 1);
    EXPECT_NE(capturedOutput.find("stderr-output"), std::string::npos);
}
