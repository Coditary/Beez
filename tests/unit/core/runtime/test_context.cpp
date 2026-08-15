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

TEST(ContextTest, LogFailureNoOpWithoutCallback)
{
    const beez::core::Context Ctx;
    Ctx.logFailure("ignored");
}

TEST(ContextTest, BuildScriptPathHonorsCustomFileName)
{
    const std::filesystem::path ProjectRoot = "/tmp/my-project";
    beez::core::Context context(ProjectRoot);
    context.setBuildScriptFileName("custom.lua");

    EXPECT_EQ(context.buildScriptPath(), ProjectRoot / "custom.lua");
}

TEST(ContextTest, EnvFilePathDefaultsToDotEnvUnderProjectRoot)
{
    const std::filesystem::path ProjectRoot = "/tmp/my-project";
    const beez::core::Context Ctx(ProjectRoot);

    EXPECT_EQ(Ctx.envFilePath(), ProjectRoot / ".env");
}

TEST(ContextTest, EnvFilePathResolvesRelativePathUnderProjectRoot)
{
    const std::filesystem::path ProjectRoot = "/tmp/my-project";
    beez::core::Context context(ProjectRoot);
    context.setEnvFilePath("config/local.env");

    EXPECT_EQ(context.envFilePath(), ProjectRoot / "config/local.env");
}

TEST(ContextTest, EnvFilePathKeepsAbsolutePath)
{
    const std::filesystem::path ProjectRoot = "/tmp/my-project";
    const std::filesystem::path Absolute = "/etc/beez.env";
    beez::core::Context context(ProjectRoot);
    context.setEnvFilePath(Absolute);

    EXPECT_EQ(context.envFilePath(), Absolute);
}

TEST(ContextTest, RecordCacheUnitInvokesRecorder)
{
    beez::core::Context context;
    bool recorded = false;
    context.setCacheStatsRecorder(
        [&recorded](const bool Hit, const double SavedSeconds)
        {
            recorded = true;
            EXPECT_TRUE(Hit);
            EXPECT_DOUBLE_EQ(SavedSeconds, 1.5);
        });
    context.recordCacheUnit(true, 1.5);
    EXPECT_TRUE(recorded);

    context.clearCacheStatsRecorder();
    context.recordCacheUnit(false);
}

TEST(ContextTest, ConsumePendingWorkerDurationReturnsStoredValueOnce)
{
    const beez::core::Context Ctx;
    EXPECT_DOUBLE_EQ(Ctx.consumePendingWorkerDuration(), 0.0);
    Ctx.setPendingWorkerDuration(2.5);
    EXPECT_DOUBLE_EQ(Ctx.consumePendingWorkerDuration(), 2.5);
    EXPECT_DOUBLE_EQ(Ctx.consumePendingWorkerDuration(), 0.0);
}
