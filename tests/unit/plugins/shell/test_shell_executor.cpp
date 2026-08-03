#include "beez/core/context.h"
#include "beez/plugin/shell/shell_executor.h"

#include <gtest/gtest.h>

TEST(ShellExecutorTest, TrueCommandReturnsZero)
{
    const beez::core::Context Ctx;
    beez::plugin::shell::ShellExecutor executor;

    EXPECT_EQ(executor.execute("true", Ctx), 0);
}

TEST(ShellExecutorTest, FalseCommandReturnsNonZero)
{
    const beez::core::Context Ctx;
    beez::plugin::shell::ShellExecutor executor;

    EXPECT_NE(executor.execute("false", Ctx), 0);
}
