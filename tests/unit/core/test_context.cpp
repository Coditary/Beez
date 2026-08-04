#include "beez/core/context.h"

#include <gtest/gtest.h>

#include <filesystem>

TEST(ContextTest, BuildScriptPathIsUnderProjectRoot)
{
    const std::filesystem::path ProjectRoot = "/tmp/my-project";
    const beez::core::Context Ctx(ProjectRoot);

    EXPECT_EQ(Ctx.buildScriptPath(), ProjectRoot / "build.lua");
}

TEST(ContextTest, ExposesProjectRoot)
{
    const std::filesystem::path ProjectRoot = "/tmp/another-project";
    const beez::core::Context Ctx(ProjectRoot);

    EXPECT_EQ(Ctx.projectRoot(), ProjectRoot);
}
