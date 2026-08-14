#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

struct DslLoadCase
{
    const char* name;
    const char* script;
    bool shouldLoad;
};

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

std::string dslFieldTestName(const ::testing::TestParamInfo<DslLoadCase>& info)
{
    std::string name = info.param.name;
    // NOLINTNEXTLINE(modernize-use-ranges) -- std::ranges::replace requires additional headers
    std::replace(name.begin(), name.end(), '.', '_');
    return name;
}

class DslFieldLoadTest : public ::testing::TestWithParam<DslLoadCase>
{
};

TEST_P(DslFieldLoadTest, MatchesExpectedLoadResult)
{
    const auto& testCase = GetParam();
    const beez::test::TempProject Project;
    Project.writeBuildLua(testCase.script);

    beez::core::Registry registry;
    EXPECT_EQ(loadScript(Project, registry), testCase.shouldLoad) << testCase.name;
}

// NOLINTBEGIN(modernize-avoid-c-arrays,modernize-use-designated-initializers,readability-identifier-naming)
const DslLoadCase StepFieldCases[] = {
    {"step.minimal_valid",
     R"(step({ name = "s", phase = "p", scope = "sc", run = "true" }))",
     true},
    {"step.only_name", R"(step({ name = "s" }))", false},
    {"step.name_phase_only", R"(step({ name = "s", phase = "p" }))", false},
    {"step.name_phase_scope_only", R"(step({ name = "s", phase = "p", scope = "sc" }))", false},
    {"step.missing_name", R"(step({ phase = "p", scope = "sc", run = "true" }))", false},
    {"step.missing_phase", R"(step({ name = "s", scope = "sc", run = "true" }))", false},
    {"step.missing_scope", R"(step({ name = "s", phase = "p", run = "true" }))", false},
    {"step.missing_run", R"(step({ name = "s", phase = "p", scope = "sc" }))", false},
    {"step.empty_table", R"(step({}))", false},
    {"step.invalid_phase_type",
     R"(step({ name = "s", phase = 42, scope = "sc", run = "true" }))",
     false},
    {"step.invalid_scope_type",
     R"(step({ name = "s", phase = "p", scope = 42, run = "true" }))",
     false},
    {"step.invalid_description_type",
     R"(step({ name = "s", phase = "p", scope = "sc", description = 42, run = "true" }))",
     false},
    {"step.optional_description",
     R"(step({ name = "s", phase = "p", scope = "sc", description = "docs", run = "true" }))",
     true},
    {"step.empty_artifact_tables",
     R"(step({ name = "s", phase = "p", scope = "sc", input = {}, output = {}, mutate = {}, run = "true" }))",
     true},
};

const DslLoadCase TaskFieldCases[] = {
    {"task.string_form", R"(task("hello", "echo hi"))", true},
    {"task.empty_string_command", R"(task("hello", ""))", true},
    {"task.single_shell_action", R"(task("hello", { "echo hi" }))", true},
    {"task.empty_action_table", R"(task("empty", {}))", false},
    {"task.invalid_action_type", R"(task("broken", { 42 }))", false},
    {"task.phase_without_registered_steps",
     R"(task("broken", { { phase = "p[sc]" } }))",
     false},
    {"task.valid_phase_scope",
     R"(step({ name = "s", phase = "p", scope = "sc", run = "true" })
task("run", { { phase = "p[sc]" } }))",
     true},
    {"task.step_ref_missing_step", R"(task("broken", { { config = { x = 1 } } }))", false},
    {"task.step_ref_unknown_step_loads", R"(task("run", { { step = "does-not-exist" } }))", false},
    {"task.step_ref_with_config",
     R"(step({ name = "s", phase = "p", scope = "sc", run = "true" })
task("run", { { step = "s", config = { flag = true } } }))",
     true},
};

const DslLoadCase WorkflowFieldCases[] = {
    {"workflow.empty", R"(workflow("run", {}))", true},
    {"workflow.valid_phase_scope",
     R"(step({ name = "s", phase = "p", scope = "sc", run = "true" })
workflow("run", { { phase = "p", scope = "sc" } }))",
     true},
    {"workflow.missing_phase_accepts_empty_string",
     R"(workflow("run", { { scope = "sc" } }))",
     false},
    {"workflow.missing_scope_accepts_empty_string",
     R"(workflow("run", { { phase = "p" } }))",
     false},
    {"workflow.parallel_rejected",
     R"(workflow("run", { { parallel = { { phase = "p", scope = "sc" } } } }))",
     false},
    {"workflow.empty_parallel_rejected", R"(workflow("run", { { parallel = {} } }))", false},
    {"workflow.rejects_string_entries", R"(workflow("run", { "ignored-task-name" }))", false},
    {"workflow.rejects_numeric_entries", R"(workflow("run", { 42 }))", false},
    {"workflow.staged_valid",
     R"lua(step({ name = "s", phase = "clean", scope = "artifacts", run = "true" })
workflow("release", {
    { "prepare", { "clean[artifacts]" } },
}))lua",
     true},
    {"workflow.staged_valid_unscoped",
     R"lua(step({ name = "s", phase = "setup", scope = "default", run = "true" })
workflow("standard", {
    { "setup", { "setup" } },
}))lua",
     true},
    {"workflow.staged_invalid_reference",
     R"lua(step({ name = "s", phase = "clean", scope = "artifacts", run = "true" })
workflow("release", {
    { "prepare", { "[artifacts]" } },
}))lua",
     false},
};

const DslLoadCase ConfigFieldCases[] = {
    {"config.empty_table",
     R"(beez.config({})
task("hello", "true"))",
     true},
    {"config.unknown_top_level_key",
     R"(beez.config({ typo_section = { enabled = true } })
task("hello", "true"))",
     true},
    {"config.unknown_nested_performance_key",
     R"(beez.config({ performance = { not_a_real_key = 1 } })
task("hello", "true"))",
     true},
    {"config.invalid_performance_type",
     R"(beez.config({ performance = "not-a-table" })
task("hello", "true"))",
     false},
    {"config.invalid_cache_path_type",
     R"(beez.config({ cache = { path = 42 } })
task("hello", "true"))",
     false},
    {"config.valid_cache_path",
     R"(beez.config({ cache = { path = ".cache-custom" } })
task("hello", "true"))",
     true},
};

const DslLoadCase MiscDslCases[] = {
    {"configure_step_before_registration",
     R"(configure_step("future", { flag = true })
step({ name = "future", phase = "p", scope = "sc", run = "true" }))",
     true},
    {"configure_step_unknown_step",
     R"(configure_step("missing", { flag = true })
task("hello", "true"))",
     false},
    {"order_unknown_steps",
     R"(order("missing-a", "missing-b")
task("hello", "true"))",
     true},
};
// NOLINTEND(modernize-avoid-c-arrays,modernize-use-designated-initializers,readability-identifier-naming)

INSTANTIATE_TEST_SUITE_P(StepFields,
                         DslFieldLoadTest,
                         ::testing::ValuesIn(StepFieldCases),
                         dslFieldTestName);

INSTANTIATE_TEST_SUITE_P(TaskFields,
                         DslFieldLoadTest,
                         ::testing::ValuesIn(TaskFieldCases),
                         dslFieldTestName);

INSTANTIATE_TEST_SUITE_P(WorkflowFields,
                         DslFieldLoadTest,
                         ::testing::ValuesIn(WorkflowFieldCases),
                         dslFieldTestName);

INSTANTIATE_TEST_SUITE_P(ConfigFields,
                         DslFieldLoadTest,
                         ::testing::ValuesIn(ConfigFieldCases),
                         dslFieldTestName);

INSTANTIATE_TEST_SUITE_P(MiscDsl,
                         DslFieldLoadTest,
                         ::testing::ValuesIn(MiscDslCases),
                         dslFieldTestName);

TEST(LuaDslFieldsTest, TaskStepReferenceToMissingStepFailsToLoad)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("run", { { step = "missing-step" } })
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("run").has_value());
}

TEST(LuaDslFieldsTest, WorkflowWithNoMatchingStepsFailsToLoad)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("run", { { phase = "missing", scope = "phase" } })
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findWorkflow("run").has_value());
}
