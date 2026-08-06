#include "beez/core/glob_pattern.hpp"
#include "beez/core/step.hpp"
#include "beez/core/step_order.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

beez::core::Step makeShellStep(const std::string& name,
                               const std::string& phase,
                               const std::string& scope,
                               std::vector<std::string> input = {},
                               std::vector<std::string> output = {},
                               std::vector<std::string> mutate = {})
{
    beez::core::Step step;
    step.name = name;
    step.phase = phase;
    step.scope = scope;
    step.shellRun = "echo " + name;
    step.input = std::move(input);
    step.output = std::move(output);
    step.mutate = std::move(mutate);
    return step;
}

class StepOrderTest : public ::testing::Test
{
  protected:
    std::unique_ptr<beez::core::IGlobMatcher> matcher = beez::core::makeSimpleGlobMatcher();
};

}  // namespace

TEST_F(StepOrderTest, OrdersProducerBeforeConsumer)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("link", "compile", "cpp", {"build/**/*.o"}, {}, {}),
        makeShellStep("compile", "compile", "cpp", {"src/**/*.cpp"}, {"build/**/*.o"}, {}),
    };

    const auto Result = beez::core::orderSteps(Steps, {}, *matcher);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(Result.value().size(), 2U);
    EXPECT_EQ(Result.value()[0].name, "compile");
    EXPECT_EQ(Result.value()[1].name, "link");
}

TEST_F(StepOrderTest, OrdersMutateBeforeConsumer)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("compile", "compile", "cpp", {"src/**/*.cpp"}, {"build/**/*.o"}, {}),
        makeShellStep("format", "compile", "cpp", {}, {}, {"src/**/*.cpp"}),
    };

    const auto Result = beez::core::orderSteps(Steps, {}, *matcher);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(Result.value().size(), 2U);
    EXPECT_EQ(Result.value()[0].name, "format");
    EXPECT_EQ(Result.value()[1].name, "compile");
}

TEST_F(StepOrderTest, ReportsMutateConflictWithoutExplicitOrder)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("cpp:format", "compile", "cpp", {}, {}, {"src/**/*.cpp"}),
        makeShellStep("cpp:lint", "compile", "cpp", {}, {}, {"src/**/*.cpp"}),
    };

    const auto Result = beez::core::orderSteps(Steps, {}, *matcher);
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error().kind, beez::core::StepOrderErrorKind::MutateConflict);
    EXPECT_NE(Result.error().message.find("order("), std::string::npos);
}

TEST_F(StepOrderTest, ResolvesMutateConflictWithExplicitOrder)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("cpp:format", "compile", "cpp", {}, {}, {"src/**/*.cpp"}),
        makeShellStep("cpp:lint", "compile", "cpp", {}, {}, {"src/**/*.cpp"}),
    };

    const std::vector<beez::core::StepOrderHint> Hints = {
        {.before = "cpp:lint", .after = "cpp:format"},
    };

    const auto Result = beez::core::orderSteps(Steps, Hints, *matcher);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(Result.value().size(), 2U);
    EXPECT_EQ(Result.value()[0].name, "cpp:lint");
    EXPECT_EQ(Result.value()[1].name, "cpp:format");
}

TEST_F(StepOrderTest, ReportsCycleError)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("a", "compile", "cpp", {"x/**"}, {"y/**"}, {}),
        makeShellStep("b", "compile", "cpp", {"y/**"}, {"x/**"}, {}),
    };

    const auto Result = beez::core::orderSteps(Steps, {}, *matcher);
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error().kind, beez::core::StepOrderErrorKind::Cycle);
}

TEST_F(StepOrderTest, StableSortByNameWhenNoDependencies)
{
    const std::vector<beez::core::Step> Steps = {
        makeShellStep("zebra", "compile", "cpp"),
        makeShellStep("alpha", "compile", "cpp"),
    };

    const auto Result = beez::core::orderSteps(Steps, {}, *matcher);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(Result.value().size(), 2U);
    EXPECT_EQ(Result.value()[0].name, "alpha");
    EXPECT_EQ(Result.value()[1].name, "zebra");
}
