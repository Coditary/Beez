#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/step_config.hpp"
#include "beez/core/step_order.hpp"
#include "beez/core/task.hpp"
#include "beez/core/task_action.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"

#include "helpers/test_helpers.hpp"
#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>

TEST(RegistryTest, FindUnknownTaskReturnsEmpty)
{
    const beez::core::Registry Registry;
    EXPECT_FALSE(Registry.findTask("missing").has_value());
}

TEST(RegistryTest, FindUnknownStepReturnsEmpty)
{
    const beez::core::Registry Registry;
    EXPECT_FALSE(Registry.findStep("missing").has_value());
}

TEST(RegistryTest, FindUnknownWorkflowReturnsEmpty)
{
    const beez::core::Registry Registry;
    EXPECT_FALSE(Registry.findWorkflow("missing").has_value());
}

TEST(RegistryTest, RegisterAndFindOrphanTask)
{
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.actions = {beez::core::makeShellAction("rm -fr app.o")};
    registry.registerTask(std::move(task));

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->name, "clean");
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectShellCommand(*Found, 0, "rm -fr app.o");
}

TEST(RegistryTest, RegisterAndFindStep)
{
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "doxygen";
    step.phase = "generate";
    step.scope = "docs";
    step.shellRun = "doxygen Doxyfile";
    registry.registerStep(std::move(step));

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

TEST(RegistryTest, RegisterTaskOverwritesExisting)
{
    beez::core::Registry registry;

    beez::core::Task first;
    first.name = "clean";
    first.actions = {beez::core::makeShellAction("rm -fr app.o")};
    registry.registerTask(std::move(first));

    beez::core::Task second;
    second.name = "clean";
    second.actions = {beez::core::makeShellAction("echo updated")};
    registry.registerTask(std::move(second));

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectShellCommand(*Found, 0, "echo updated");
}

TEST(RegistryTest, StepsForPhaseFiltersByPhaseAndScope)
{
    beez::core::Registry registry;

    beez::core::Step docsStep;
    docsStep.name = "doxygen";
    docsStep.phase = "generate";
    docsStep.scope = "docs";
    docsStep.shellRun = "doxygen";
    registry.registerStep(std::move(docsStep));

    beez::core::Step codeStep;
    codeStep.name = "protobuf";
    codeStep.phase = "generate";
    codeStep.scope = "code";
    codeStep.shellRun = "protoc";
    registry.registerStep(std::move(codeStep));

    const auto DocsMatches = registry.stepsForPhase("generate", "docs");
    ASSERT_TRUE(DocsMatches.hasValue());
    ASSERT_EQ(DocsMatches.value().size(), 1U);
    EXPECT_EQ(DocsMatches.value().front().name, "doxygen");

    const auto CodeMatches = registry.stepsForPhase("generate", "code");
    ASSERT_TRUE(CodeMatches.hasValue());
    ASSERT_EQ(CodeMatches.value().size(), 1U);
    EXPECT_EQ(CodeMatches.value().front().name, "protobuf");
}

TEST(RegistryTest, StepsForPhaseReturnsEmptyWhenNoMatch)
{
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "compile";
    step.phase = "compile";
    step.scope = "code";
    step.shellRun = "make";
    registry.registerStep(std::move(step));

    const auto NoGenerateDocs = registry.stepsForPhase("generate", "docs");
    ASSERT_TRUE(NoGenerateDocs.hasValue());
    EXPECT_TRUE(NoGenerateDocs.value().empty());

    const auto NoCompileDocs = registry.stepsForPhase("compile", "docs");
    ASSERT_TRUE(NoCompileDocs.hasValue());
    EXPECT_TRUE(NoCompileDocs.value().empty());
}

TEST(RegistryTest, StepsForPhaseWildcardScopeMatchesAllScopes)
{
    beez::core::Registry registry;

    beez::core::Step docsStep;
    docsStep.name = "doxygen";
    docsStep.phase = "generate";
    docsStep.scope = "docs";
    docsStep.shellRun = "doxygen";
    registry.registerStep(std::move(docsStep));

    beez::core::Step codeStep;
    codeStep.name = "protobuf";
    codeStep.phase = "generate";
    codeStep.scope = "code";
    codeStep.shellRun = "protoc";
    registry.registerStep(std::move(codeStep));

    const auto Matches = registry.stepsForPhase("generate", "*");
    ASSERT_TRUE(Matches.hasValue());
    ASSERT_EQ(Matches.value().size(), 2U);
}

TEST(RegistryTest, ScopesForPhaseReturnsUniqueSortedScopes)
{
    beez::core::Registry registry;

    beez::core::Step docsStep;
    docsStep.name = "doxygen";
    docsStep.phase = "generate";
    docsStep.scope = "docs";
    docsStep.shellRun = "doxygen";
    registry.registerStep(std::move(docsStep));

    beez::core::Step codeStep;
    codeStep.name = "protobuf";
    codeStep.phase = "generate";
    codeStep.scope = "code";
    codeStep.shellRun = "protoc";
    registry.registerStep(std::move(codeStep));

    const auto Scopes = registry.scopesForPhase("generate");
    ASSERT_EQ(Scopes.size(), 2U);
    EXPECT_EQ(Scopes[0], "code");
    EXPECT_EQ(Scopes[1], "docs");
}

TEST(RegistryTest, RegisterAndFindWorkflow)
{
    beez::core::Registry registry;

    beez::core::Workflow workflow;
    workflow.name = "build";
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}}});
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}}});
    registry.registerWorkflow(std::move(workflow));

    const auto FoundWorkflow = beez::test::requireWorkflow(registry, "build");
    ASSERT_TRUE(FoundWorkflow.has_value());
    if (!FoundWorkflow)
    {
        return;
    }
    ASSERT_EQ(FoundWorkflow->steps.size(), 2U);
    beez::test::expectSequentialStep(FoundWorkflow->steps[0], "generate", "code");
    beez::test::expectSequentialStep(FoundWorkflow->steps[1], "compile", "code");
}

TEST(RegistryTest, RegisterWorkflowWithParallelPhases)
{
    beez::core::Registry registry;

    beez::core::Workflow workflow;
    workflow.name = "ci";
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "generate", .scope = "docs"},
                        beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}}});
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}}});
    registry.registerWorkflow(std::move(workflow));

    const auto FoundWorkflow = beez::test::requireWorkflow(registry, "ci");
    ASSERT_TRUE(FoundWorkflow.has_value());
    if (!FoundWorkflow)
    {
        return;
    }
    ASSERT_EQ(FoundWorkflow->steps.size(), 2U);
    beez::test::expectParallelStep(FoundWorkflow->steps[0],
                                   {{"generate", "docs"}, {"generate", "code"}});
    beez::test::expectSequentialStep(FoundWorkflow->steps[1], "compile", "code");
}

TEST(RegistryTest, ConfigureStepBeforeRegistrationAppliesOnRegister)
{
    beez::core::Registry registry;

    registry.configureStep("shader", beez::test::makeTestConfig("external"));

    beez::core::Step step;
    step.name = "shader";
    step.phase = "generate";
    step.scope = "code";
    step.shellRun = "echo shader";
    registry.registerStep(std::move(step));

    beez::test::expectStepConfigTag(registry, "shader", "external");
}

TEST(RegistryTest, ConfigureStepAfterRegistrationMergesIntoStep)
{
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "shader";
    step.phase = "generate";
    step.scope = "code";
    step.shellRun = "echo shader";
    step.config = beez::test::makeTestConfig("inline");
    registry.registerStep(std::move(step));

    registry.configureStep("shader", beez::test::makeTestConfig("external"));

    beez::test::expectStepConfigTag(registry, "shader", "inline+external");
}

TEST(RegistryTest, ConfigureStepOverridesInlineStepDefault)
{
    beez::core::Registry registry;

    registry.configureStep("shader", beez::test::makeTestConfig("external"));

    beez::core::Step step;
    step.name = "shader";
    step.phase = "generate";
    step.scope = "code";
    step.shellRun = "echo shader";
    step.config = beez::test::makeTestConfig("inline");
    registry.registerStep(std::move(step));

    beez::test::expectStepConfigTag(registry, "shader", "inline+external");
}

TEST(RegistryTest, StepsForPhaseOrdersByArtifactDependencies)
{
    beez::core::Registry registry;

    beez::core::Step linkStep;
    linkStep.name = "link";
    linkStep.phase = "compile";
    linkStep.scope = "cpp";
    linkStep.shellRun = "echo link";
    linkStep.input = {"build/**/*.o"};
    registry.registerStep(std::move(linkStep));

    beez::core::Step compileStep;
    compileStep.name = "compile";
    compileStep.phase = "compile";
    compileStep.scope = "cpp";
    compileStep.shellRun = "echo compile";
    compileStep.input = {"src/**/*.cpp"};
    compileStep.output = {"build/**/*.o"};
    registry.registerStep(std::move(compileStep));

    const auto Ordered = registry.stepsForPhase("compile", "cpp");
    ASSERT_TRUE(Ordered.hasValue());
    ASSERT_EQ(Ordered.value().size(), 2U);
    EXPECT_EQ(Ordered.value()[0].name, "compile");
    EXPECT_EQ(Ordered.value()[1].name, "link");
}

TEST(RegistryTest, StepsForPhaseReturnsMutateConflictError)
{
    beez::core::Registry registry;

    beez::core::Step formatStep;
    formatStep.name = "cpp:format";
    formatStep.phase = "compile";
    formatStep.scope = "cpp";
    formatStep.shellRun = "echo format";
    formatStep.mutate = {"src/**/*.cpp"};
    registry.registerStep(std::move(formatStep));

    beez::core::Step lintStep;
    lintStep.name = "cpp:lint";
    lintStep.phase = "compile";
    lintStep.scope = "cpp";
    lintStep.shellRun = "echo lint";
    lintStep.mutate = {"src/**/*.cpp"};
    registry.registerStep(std::move(lintStep));

    const auto Ordered = registry.stepsForPhase("compile", "cpp");
    ASSERT_FALSE(Ordered.hasValue());
    EXPECT_EQ(Ordered.error().kind, beez::core::StepOrderErrorKind::MutateConflict);
}

TEST(RegistryTest, RegisterStepOrderResolvesMutateConflict)
{
    beez::core::Registry registry;
    registry.registerStepOrder("cpp:lint", "cpp:format");

    beez::core::Step formatStep;
    formatStep.name = "cpp:format";
    formatStep.phase = "compile";
    formatStep.scope = "cpp";
    formatStep.shellRun = "echo format";
    formatStep.mutate = {"src/**/*.cpp"};
    registry.registerStep(std::move(formatStep));

    beez::core::Step lintStep;
    lintStep.name = "cpp:lint";
    lintStep.phase = "compile";
    lintStep.scope = "cpp";
    lintStep.shellRun = "echo lint";
    lintStep.mutate = {"src/**/*.cpp"};
    registry.registerStep(std::move(lintStep));

    const auto Ordered = registry.stepsForPhase("compile", "cpp");
    ASSERT_TRUE(Ordered.hasValue());
    ASSERT_EQ(Ordered.value().size(), 2U);
    EXPECT_EQ(Ordered.value()[0].name, "cpp:lint");
    EXPECT_EQ(Ordered.value()[1].name, "cpp:format");
}
