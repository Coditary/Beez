#include "beez/cli/list_formatter.hpp"

#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

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

TEST(ListFormatterTest, FormatsUniqueSortedPhaseNamesFromSteps)
{
    beez::core::Registry registry;

    beez::core::Step compileStep;
    compileStep.name = "compile:lua";
    compileStep.phase = "compile";
    compileStep.scope = "lua";
    registry.registerStep(std::move(compileStep));

    beez::core::Step generateStep;
    generateStep.name = "generate:code";
    generateStep.phase = "generate";
    generateStep.scope = "code";
    registry.registerStep(std::move(generateStep));

    beez::core::Step generateDocsStep;
    generateDocsStep.name = "generate:docs";
    generateDocsStep.phase = "generate";
    generateDocsStep.scope = "docs";
    registry.registerStep(std::move(generateDocsStep));

    const auto Names = beez::cli::collectEntityNames(registry, "phases");
    ASSERT_EQ(Names.size(), 2U);
    EXPECT_EQ(Names[0], "compile");
    EXPECT_EQ(Names[1], "generate");
}
