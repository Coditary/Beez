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

TEST(LuaShellPipelineTest, StepRunFunctionReceivesConfigThroughContext)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure_step("shader", {
    shader_version = "450",
    optimize = true,
    defines = { "DEBUG", "USE_GLSL" },
    output_dir = "build/shaders",
})
step({
    name = "shader",
    phase = "generate",
    scope = "code",
    run = function(ctx)
        local config = ctx.get_config()
        if config.shader_version ~= "450" then
            return 1
        end
        if not config.optimize then
            return 1
        end
        if config.defines[1] ~= "DEBUG" then
            return 1
        end
        if config.output_dir ~= "build/shaders" then
            return 1
        end
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("shader").hasValue());
}

TEST(LuaShellPipelineTest, WorkflowExecutesRegisteredSteps)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = "touch .compiled",
})
workflow("build", {
    { phase = "compile", scope = "code" },
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    EXPECT_TRUE(runtime.registry().findWorkflow("build").has_value());
    EXPECT_TRUE(orchestrator.run("build").hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".compiled"));
}
