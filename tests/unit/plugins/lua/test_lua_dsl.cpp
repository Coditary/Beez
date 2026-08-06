#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(LuaDslTest, LoadsOrphanTaskFromStringForm)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("clean", "rm -fr app.o")
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectShellCommand(*Found, 0, "rm -fr app.o");
}

TEST(LuaDslTest, LoadsTaskWithMultipleCommands)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("hello", {
    "echo Hello World",
    "echo Goodbye World",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "hello");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->actions.size(), 2U);
    beez::test::expectShellCommand(*Found, 0, "echo Hello World");
    beez::test::expectShellCommand(*Found, 1, "echo Goodbye World");
}

TEST(LuaDslTest, LoadsStepFromTableForm)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "doxygen",
    phase = "generate",
    scope = "docs",
    run = "doxygen Doxyfile",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("doxygen");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->phase, "generate");
    EXPECT_EQ(Found->scope, "docs");
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "doxygen Doxyfile");
}

TEST(LuaDslTest, LoadsStepDescriptionWhenProvided)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "doxygen",
    phase = "generate",
    scope = "docs",
    description = "Generate API documentation",
    run = "doxygen Doxyfile",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("doxygen");
    ASSERT_TRUE(Found.has_value());
    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    ASSERT_TRUE(Found->description.has_value());
    EXPECT_EQ(Found->description.value(), "Generate API documentation");
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(LuaDslTest, StepWithoutConfigHasNoConfig)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "plain",
    phase = "generate",
    scope = "code",
    run = "echo plain",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("plain");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_FALSE(Found->hasConfig());
}

TEST(LuaDslTest, LoadsStepWithInlineConfig)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "shader",
    phase = "generate",
    scope = "code",
    config = {
        shader_version = "450",
        optimize = true,
    },
    run = "echo shader",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    beez::test::expectStepHasConfig(registry, "shader");
}

TEST(LuaDslTest, LoadsConfigureStepBeforeStepRegistration)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure_step("shader", {
    shader_version = "450",
    optimize = true,
})
step({
    name = "shader",
    phase = "generate",
    scope = "code",
    run = "echo shader",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    beez::test::expectStepHasConfig(registry, "shader");
}

TEST(LuaDslTest, LoadsConfigureStepAfterStepRegistration)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "shader",
    phase = "generate",
    scope = "code",
    config = {
        output_dir = "build/shaders",
    },
    run = "echo shader",
})
configure_step("shader", {
    shader_version = "450",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    beez::test::expectStepHasConfig(registry, "shader");
}

TEST(LuaDslTest, ReturnsFalseWhenStepConfigIsNotTable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "broken",
    phase = "generate",
    scope = "code",
    config = "not-a-table",
    run = "echo broken",
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findStep("broken").has_value());
}

TEST(LuaDslTest, LoadsTaskWithStepInvocation)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "cpp:compile",
    phase = "compile",
    scope = "code",
    run = "echo compile",
})
task("full_build", {
    "echo start",
    { name = "cpp:compile" },
    "echo done",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "full_build");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    beez::test::expectMixedTaskWithStepInvocation(*Found);
}

TEST(LuaDslTest, LoadsTaskWithStepInvocationInlineConfig)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "cpp:compile",
    phase = "compile",
    scope = "code",
    run = "echo compile",
})
task("full_build", {
    { name = "cpp:compile", config = { optimize = "-O3" } },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "full_build");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectStepInvocation(*Found, 0, "cpp:compile", true);
}

TEST(LuaDslTest, ReturnsFalseWhenTaskStepInvocationMissingName)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("broken", {
    { config = { optimize = "-O3" } },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("broken").has_value());
}

TEST(LuaDslTest, ReturnsFalseWhenTaskStepInvocationConfigIsNotTable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("broken", {
    { name = "cpp:compile", config = "not-a-table" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("broken").has_value());
}

TEST(LuaDslTest, LoadsSequentialWorkflow)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireWorkflow(registry, "build");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->steps.size(), 2U);
    beez::test::expectSequentialStep(Found->steps[0], "generate", "code");
    beez::test::expectSequentialStep(Found->steps[1], "compile", "code");
}

TEST(LuaDslTest, LoadsWorkflowWithParallelStep)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("ci", {
    { parallel = {
        { phase = "generate", scope = "docs" },
        { phase = "generate", scope = "code" },
    }},
    { phase = "compile", scope = "code" },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireWorkflow(registry, "ci");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->steps.size(), 2U);
    beez::test::expectParallelStep(Found->steps[0], {{"generate", "docs"}, {"generate", "code"}});
    beez::test::expectSequentialStep(Found->steps[1], "compile", "code");
}

TEST(LuaDslTest, ReturnsFalseForSyntaxError)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("this is not valid lua {{{");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("clean").has_value());
}

TEST(LuaDslTest, ReturnsFalseWhenStepTableMissingRun)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({ name = "broken", phase = "generate", scope = "docs" })
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findStep("broken").has_value());
}

TEST(LuaDslTest, ReturnsFalseWhenTaskTableIsNotCommandList)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("broken", { phase = "generate", scope = "docs", run = "true" })
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("broken").has_value());
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- raw string literals inflate metric
TEST(LuaDslTest, LoadsStepArtifactFields)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    input = { "src/**/*.cpp" },
    output = { "build/**/*.o" },
    run = "echo compile",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("compile");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->input.size(), 1U);
    EXPECT_EQ(Found->input[0], "src/**/*.cpp");
    ASSERT_EQ(Found->output.size(), 1U);
    EXPECT_EQ(Found->output[0], "build/**/*.o");
    EXPECT_TRUE(Found->mutate.empty());
}

TEST(LuaDslTest, LoadsStepMutateField)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "cpp:format",
    phase = "compile",
    scope = "cpp",
    mutate = { "src/**/*.cpp" },
    run = "echo format",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("cpp:format");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->mutate.size(), 1U);
    EXPECT_EQ(Found->mutate[0], "src/**/*.cpp");
}

TEST(LuaDslTest, LoadsOrderDeclaration)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
order("cpp:lint", "cpp:format")
step({
    name = "cpp:format",
    phase = "compile",
    scope = "cpp",
    mutate = { "src/**/*.cpp" },
    run = "echo format",
})
step({
    name = "cpp:lint",
    phase = "compile",
    scope = "cpp",
    mutate = { "src/**/*.cpp" },
    run = "echo lint",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Ordered = registry.stepsForPhase("compile", "cpp");
    ASSERT_TRUE(Ordered.hasValue());
    ASSERT_EQ(Ordered.value().size(), 2U);
    EXPECT_EQ(Ordered.value()[0].name, "cpp:lint");
    EXPECT_EQ(Ordered.value()[1].name, "cpp:format");
}

TEST(LuaDslTest, ReturnsFalseWhenArtifactFieldIsNotTable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "broken",
    phase = "compile",
    scope = "cpp",
    input = "src/**/*.cpp",
    run = "echo broken",
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findStep("broken").has_value());
}

TEST(LuaDslTest, LoadsBuildScriptWithoutBeezEnvWhenDotEnvMissing)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("hello", "echo no-env-overhead")
)");

    beez::core::Registry registry;
    EXPECT_TRUE(loadScript(Project, registry));
}

TEST(LuaDslTest, BeezEnvReadsValueFromDotEnvFile)
{
    const beez::test::TempProject Project;
    Project.writeDotEnv("MY_ENV=secret-value\n");
    Project.writeBuildLua(R"(
task("show-env", "echo " .. beez.env("MY_ENV"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "show-env");
    ASSERT_TRUE(Found.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    beez::test::expectShellCommand(*Found, 0, "echo secret-value");
}

TEST(LuaDslTest, BeezEnvReturnsNilWhenVariableIsMissing)
{
    const beez::test::TempProject Project;
    Project.writeDotEnv("OTHER=value\n");
    Project.writeBuildLua(R"(
step({
    name = "check-env",
    phase = "test",
    scope = "code",
    run = function()
        if beez.env("MISSING_ENV") ~= nil then
            error("expected nil for missing env variable")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    ASSERT_TRUE(registry.findStep("check-env").has_value());
}

TEST(LuaDslTest, BeezEnvDoesNotReadDotEnvUntilCalled)
{
    const beez::test::TempProject Project;
    Project.writeDotEnv("this is not valid dotenv {{{\n");
    Project.writeBuildLua(R"(
task("hello", "echo without env access")
)");

    beez::core::Registry registry;
    EXPECT_TRUE(loadScript(Project, registry));
}
