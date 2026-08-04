#include "beez/core/orchestrator.h"

#include "helpers/beez_runtime.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(OrchestratorPipelineTest, LoadsAndRunsOrphanTaskWithRealShell)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("mark", "touch .beez-ran")
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    const auto LoadResult = orchestrator.loadBuildScript();
    ASSERT_TRUE(LoadResult.hasValue());

    const auto RunResult = orchestrator.run("mark");
    ASSERT_TRUE(RunResult.hasValue());
    EXPECT_EQ(RunResult.value(), 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".beez-ran"));
}

TEST(OrchestratorPipelineTest, LoadsAndRunsStepByName)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "docs",
    phase = "generate",
    scope = "docs",
    run = "touch .docs-ran",
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    const auto RunResult = orchestrator.runStep("docs");
    ASSERT_TRUE(RunResult.hasValue());
    EXPECT_EQ(RunResult.value(), 0);
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".docs-ran"));
}

TEST(OrchestratorPipelineTest, MissingBuildScriptReturnsNotFound)
{
    const beez::test::TempProject Project;
    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    const auto LoadResult = orchestrator.loadBuildScript();
    ASSERT_FALSE(LoadResult.hasValue());
    EXPECT_EQ(LoadResult.error(), beez::core::OrchestratorError::BuildScriptNotFound);
}

TEST(OrchestratorPipelineTest, InvalidBuildScriptReturnsLoadFailed)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("this is not valid lua {{{");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    const auto LoadResult = orchestrator.loadBuildScript();
    ASSERT_FALSE(LoadResult.hasValue());
    EXPECT_EQ(LoadResult.error(), beez::core::OrchestratorError::BuildScriptLoadFailed);
    EXPECT_FALSE(runtime.registry().findTask("clean").has_value());
}

TEST(OrchestratorPipelineTest, UnknownTaskReturnsNotFound)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("known", "true")
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    const auto RunResult = orchestrator.run("missing");
    ASSERT_FALSE(RunResult.hasValue());
    EXPECT_EQ(RunResult.error(), beez::core::OrchestratorError::NotFound);
}

TEST(OrchestratorPipelineTest, WorkflowExecutionRunsMatchingSteps)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "gen-code",
    phase = "generate",
    scope = "code",
    run = "touch .generated",
})
workflow("build", {
    { phase = "generate", scope = "code" },
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());

    const auto RunResult = orchestrator.run("build");
    ASSERT_TRUE(RunResult.hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".generated"));
}
