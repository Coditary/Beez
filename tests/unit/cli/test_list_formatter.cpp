#include "beez/cli/list_formatter.hpp"

#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(ListFormatterTest, FormatsSortedTaskNames)
{
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {.name = "beta"});
    registry.registerTask(beez::core::Task {.name = "alpha"});

    const auto Names = beez::cli::collectEntityNames(registry, "tasks");
    ASSERT_EQ(Names.size(), 2U);
    EXPECT_EQ(Names[0], "alpha");
    EXPECT_EQ(Names[1], "beta");

    const std::string Formatted = beez::cli::formatEntityList("tasks", Names);
    EXPECT_NE(Formatted.find("tasks:\n"), std::string::npos);
    EXPECT_NE(Formatted.find("  alpha\n"), std::string::npos);
    EXPECT_NE(Formatted.find("  beta\n"), std::string::npos);
}

TEST(ListFormatterTest, FormatsWorkflowNames)
{
    beez::core::Registry registry;
    registry.registerWorkflow(beez::core::Workflow {.name = "build"});

    const auto Names = beez::cli::collectEntityNames(registry, "workflows");
    ASSERT_EQ(Names.size(), 1U);
    EXPECT_EQ(Names[0], "build");
}
