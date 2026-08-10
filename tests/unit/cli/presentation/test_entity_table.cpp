#include "beez/cli/presentation/entity_table.hpp"

#include "beez/core/model/step.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/registry/registry.hpp"

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

    const std::string Formatted = beez::cli::formatEntityList(registry, "tasks");
    EXPECT_NE(Formatted.find("tasks:\n\n"), std::string::npos);
    EXPECT_NE(Formatted.find("Name"), std::string::npos);
    EXPECT_NE(Formatted.find("alpha"), std::string::npos);
    EXPECT_NE(Formatted.find("beta"), std::string::npos);
    EXPECT_EQ(Formatted.find("Actions"), std::string::npos);
}

TEST(ListFormatterTest, FormatsWorkflowNames)
{
    beez::core::Registry registry;
    registry.registerWorkflow(beez::core::Workflow {.name = "build"});

    const auto Names = beez::cli::collectEntityNames(registry, "workflows");
    ASSERT_EQ(Names.size(), 1U);
    EXPECT_EQ(Names[0], "build");

    const std::string Formatted = beez::cli::formatEntityList(registry, "workflows");
    EXPECT_NE(Formatted.find("Name"), std::string::npos);
    EXPECT_NE(Formatted.find("build"), std::string::npos);
    EXPECT_EQ(Formatted.find("Plan"), std::string::npos);
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

    const std::string Formatted = beez::cli::formatEntityList(registry, "phases");
    EXPECT_NE(Formatted.find("Scopes"), std::string::npos);
    EXPECT_NE(Formatted.find("[lua]"), std::string::npos);
    EXPECT_NE(Formatted.find("[code, docs]"), std::string::npos);
}

TEST(ListFormatterTest, FormatsStepMetadataTable)
{
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "lint:cpp";
    step.phase = "lint";
    step.scope = "cpp";
    step.description = "Run C++ linters";
    registry.registerStep(std::move(step));

    const std::string Formatted = beez::cli::formatEntityList(registry, "steps");
    EXPECT_NE(Formatted.find("lint:cpp"), std::string::npos);
    EXPECT_NE(Formatted.find("lint"), std::string::npos);
    EXPECT_NE(Formatted.find("cpp"), std::string::npos);
    EXPECT_NE(Formatted.find("Run C++ linters"), std::string::npos);
}
