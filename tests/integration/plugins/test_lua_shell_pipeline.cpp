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

TEST(LuaShellPipelineTest, TaskPhaseInvocationExecutesRegisteredSteps)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = "touch .task-phase-compiled",
})
task("full_build", {
    "touch .task-phase-started",
    { phase = "compile", scope = "code" },
    "touch .task-phase-done",
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.run("full_build").hasValue());

    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-phase-started"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-phase-compiled"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-phase-done"));
}

TEST(LuaShellPipelineTest, TaskStepInvocationUsesConfigureStepOverStepDefault)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("full_build", {
    { step = "compile:lua" },
})
configure_step("compile:lua", {
    output_dir = "build",
})
step({
    name = "compile:lua",
    phase = "compile",
    scope = "code",
    config = {
        output_dir = "build/shaders-out",
    },
    run = function(ctx)
        local config = ctx.get_config()
        if config.output_dir ~= "build" then
            return 1
        end
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.run("full_build").hasValue());
}

TEST(LuaShellPipelineTest, TaskWithStepInvocationExecutesRegisteredStep)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "cpp:compile",
    phase = "compile",
    scope = "code",
    run = "touch .task-compiled",
})
task("full_build", {
    "touch .task-started",
    { step = "cpp:compile" },
    "touch .task-done",
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.run("full_build").hasValue());

    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-started"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-compiled"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / ".task-done"));
}

TEST(LuaShellPipelineTest, StepSpawnExecutesWorkerCommands)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = function(ctx)
        local job = ctx:spawn({
            cmd = "mkdir -p build && touch build/main.o",
            outputs = { "build/main.o" },
        })
        return ctx:wait(job, { exitCode = true }).exitCode
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));
}

TEST(LuaShellPipelineTest, StepSpawnWaitAllExecutesMultipleWorkers)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = function(ctx)
        local jobs = {}
        local job_a = ctx:spawn({
            cmd = "mkdir -p build && touch build/a.o",
            outputs = { "build/a.o" },
        })
        local job_b = ctx:spawn({
            cmd = "mkdir -p build && touch build/b.o",
            outputs = { "build/b.o" },
        })
        jobs[1] = job_a
        jobs[2] = job_b
        ctx:wait_all(jobs)
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "a.o"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "b.o"));
}

TEST(LuaShellPipelineTest, StepAutoDrainsWorkersWithoutExplicitWait)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = function(ctx)
        ctx:spawn({
            cmd = "mkdir -p build && touch build/main.o",
            outputs = { "build/main.o" },
        })
        return 0
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));
}

TEST(LuaShellPipelineTest, StepSpawnSupportsMultipleCommands)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "code",
    run = function(ctx)
        local job = ctx:spawn({
            cmd = {
                "mkdir -p build",
                "touch build/main.o",
                "touch build/main.done",
            },
            outputs = { "build/main.o", "build/main.done" },
        })
        return ctx:wait(job, { exitCode = true }).exitCode
    end,
})
)");

    beez::test::BeezRuntime runtime(Project.path());
    auto orchestrator = runtime.orchestrator();

    ASSERT_TRUE(orchestrator.loadBuildScript().hasValue());
    ASSERT_TRUE(orchestrator.runStep("compile").hasValue());
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.o"));
    EXPECT_TRUE(std::filesystem::exists(Project.path() / "build" / "main.done"));
}
