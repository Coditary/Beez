#include "beez/core/model/step_config.hpp"
#include "beez/core/runtime/context.hpp"

#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

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

TEST(ContextTest, GetConfigReturnsNullWhenNoAccessor)
{
    const beez::core::Context Ctx;
    EXPECT_EQ(Ctx.getConfig(), nullptr);
}

TEST(ContextTest, GetConfigReturnsValueFromAccessor)
{
    beez::core::Context context;
    auto stepConfig = beez::test::makeTestConfig("lazy");
    context.setStepConfigAccessor(
        [stepConfig = std::move(stepConfig)]() -> beez::core::StepConfigPtr { return stepConfig; });

    const auto Resolved = context.getConfig();
    ASSERT_NE(Resolved, nullptr);
    const auto* typed = dynamic_cast<const beez::test::TestStepConfig*>(Resolved.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->tag(), "lazy");
}

TEST(ContextTest, LogFailureInvokesCallback)
{
    beez::core::Context context;
    std::string captured;
    context.setFailureLogCallback([&captured](const std::string_view Message)
                                  { captured = Message; });

    context.logFailure("lint issue");

    EXPECT_EQ(captured, "lint issue");
}
