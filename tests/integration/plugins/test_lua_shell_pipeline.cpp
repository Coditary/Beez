#include "helpers/beez_runtime.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>

TEST(LuaShellPipelineTest, LuaRegisteredTaskExecutesThroughShellExecutor)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("write-marker", "touch integration-marker.txt")
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.run("write-marker").hasValue());

    EXPECT_TRUE(std::filesystem::exists(Project.path() / "integration-marker.txt"));
}

TEST(LuaShellPipelineTest, MultipleRegisteredTasksAreRunnable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("first", "touch .first")
task("second", "touch .second")
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(runtime.registry().findTask("first").has_value());
    ASSERT_TRUE(runtime.registry().findTask("second").has_value());

    ASSERT_TRUE(orchestrator.run("first").hasValue());
    ASSERT_TRUE(orchestrator.run("second").hasValue());

    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".first"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".second"));
}

TEST(LuaShellPipelineTest, WorkflowIsRegisteredButNotExecutable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("compile", { phase = "compile", scope = "code", run = "true" })
workflow("build", {
    { phase = "compile", scope = "code" },
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    EXPECT_TRUE(runtime.registry().findWorkflow("build").has_value());
    EXPECT_FALSE(orchestrator.run("build").hasValue());
    EXPECT_TRUE(orchestrator.run("compile").hasValue());
}
