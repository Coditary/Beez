#include "beez/cli/presentation/name_suggestion.hpp"

#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/registry/registry.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(NameSuggestionTest, SuggestsCloseTaskName)
{
    const std::vector<std::string> Names = {"build", "quality", "test"};
    const auto Suggestion = beez::cli::suggestSimilarName("biuld", Names);
    ASSERT_TRUE(Suggestion.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Suggestion, "build");
}

TEST(NameSuggestionTest, DoesNotSuggestWhenNameIsTooDifferent)
{
    const std::vector<std::string> Names = {"known"};
    const auto Suggestion = beez::cli::suggestSimilarName("missing", Names);
    EXPECT_FALSE(Suggestion.has_value());
}

TEST(NameSuggestionTest, CollectsTasksAndWorkflows)
{
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {.name = "build"});
    registry.registerWorkflow(beez::core::Workflow {.name = "quality"});

    const auto Names = beez::cli::collectRunnableNames(registry);
    ASSERT_EQ(Names.size(), 2U);
    EXPECT_EQ(Names[0], "build");
    EXPECT_EQ(Names[1], "quality");
}

TEST(NameSuggestionTest, FormatsDidYouMeanMessage)
{
    EXPECT_EQ(beez::cli::formatDidYouMean("build"), "Did you mean 'build'?");
}
