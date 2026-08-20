#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_order.hpp"
#include "beez/core/registry/step_resolution.hpp"

#include "helpers/test_helpers.hpp"
#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
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
    beez::test::expectShellCommand(Found, 0, "rm -fr app.o");
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

TEST(RegistryTest, RegisterTaskRejectsDuplicate)
{
    beez::core::Registry registry;

    beez::core::Task first;
    first.name = "clean";
    first.actions = {beez::core::makeShellAction("rm -fr app.o")};
    registry.registerTask(std::move(first));

    beez::core::Task second;
    second.name = "clean";
    second.actions = {beez::core::makeShellAction("echo updated")};
    EXPECT_THROW(registry.registerTask(std::move(second)), std::runtime_error);

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectShellCommand(Found, 0, "rm -fr app.o");
}

TEST(RegistryTest, RegisterStepRejectsDuplicate)
{
    beez::core::Registry registry;

    beez::core::Step first;
    first.name = "doxygen";
    first.phase = "generate";
    first.scope = "docs";
    first.shellRun = "echo first";
    registry.registerStep(std::move(first));

    beez::core::Step second;
    second.name = "doxygen";
    second.phase = "generate";
    second.scope = "docs";
    second.shellRun = "echo second";
    EXPECT_THROW(registry.registerStep(std::move(second)), std::runtime_error);

    const auto Found = registry.findStep("doxygen");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "echo first");
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
        .invocation = beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}});
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocation = beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}});
    registry.registerWorkflow(std::move(workflow));

    const auto FoundWorkflow = beez::test::requireWorkflow(registry, "build");
    ASSERT_TRUE(FoundWorkflow.has_value());
    if (!FoundWorkflow)
    {
        return;
    }
    ASSERT_EQ(FoundWorkflow->steps.size(), 2U);
    beez::test::expectWorkflowStep(FoundWorkflow->steps[0], "generate", "code");
    beez::test::expectWorkflowStep(FoundWorkflow->steps[1], "compile", "code");
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

TEST(RegistryTest, PluginStepsAllowDuplicateShortNamesWhenQualified)
{
    beez::core::Registry registry;

    beez::core::Step tidyStep;
    tidyStep.name = "check";
    tidyStep.phase = "qa";
    tidyStep.scope = "code";
    tidyStep.shellRun = "echo tidy";
    registry.registerPluginStep(std::move(tidyStep), "coditary", "clang-tidy");

    beez::core::Step cppcheckStep;
    cppcheckStep.name = "check";
    cppcheckStep.phase = "qa";
    cppcheckStep.scope = "code";
    cppcheckStep.shellRun = "echo cppcheck";
    registry.registerPluginStep(std::move(cppcheckStep), "coditary", "cppcheck");

    const auto Tidy = registry.resolveStep("coditary/clang-tidy:check");
    ASSERT_TRUE(Tidy.hasValue());
    ASSERT_TRUE(Tidy.value().hasShellRun());
    EXPECT_EQ(Tidy.value().shellRun.value_or(""), "echo tidy");

    const auto Cppcheck = registry.resolveStep("cppcheck:check");
    ASSERT_TRUE(Cppcheck.hasValue());
    ASSERT_TRUE(Cppcheck.value().hasShellRun());
    EXPECT_EQ(Cppcheck.value().shellRun.value_or(""), "echo cppcheck");

    const auto Ambiguous = registry.resolveStep("check");
    ASSERT_FALSE(Ambiguous.hasValue());
    EXPECT_EQ(Ambiguous.error().error, beez::core::StepResolutionError::Ambiguous);
    ASSERT_EQ(Ambiguous.error().candidates.size(), 2U);
}

TEST(RegistryTest, ResolvesVersionedPluginStepReference)
{
    beez::core::Registry registry;

    beez::core::Step compileStep;
    compileStep.name = "compile:code";
    compileStep.phase = "build";
    compileStep.scope = "code";
    compileStep.shellRun = "echo versioned";
    registry.registerPluginStep(std::move(compileStep), "coditary", "clang", std::string {"1.0.0"});

    const auto Resolved = registry.resolveStep("clang:compile@1.0.0");
    ASSERT_TRUE(Resolved.hasValue());
    EXPECT_EQ(Resolved.value().shellRun.value_or(""), "echo versioned");

    const auto Qualified = registry.resolveStep("coditary/clang:compile@1.0.0");
    ASSERT_TRUE(Qualified.hasValue());
    EXPECT_EQ(Qualified.value().shellRun.value_or(""), "echo versioned");

    const auto Unversioned = registry.resolveStep("clang:compile");
    ASSERT_TRUE(Unversioned.hasValue());
    EXPECT_EQ(Unversioned.value().shellRun.value_or(""), "echo versioned");
}

TEST(RegistryTest, InstalledPluginVersionKeepsVersionedAliasesOnly)
{
    beez::core::Registry registry;

    beez::core::Step compileStep;
    compileStep.name = "compile:code";
    compileStep.phase = "build";
    compileStep.scope = "code";
    compileStep.shellRun = "echo versioned";
    registry.registerPluginStep(
        std::move(compileStep), "coditary", "clang", std::string {"1.0.0"}, false);

    const auto Versioned = registry.resolveStep("clang:compile@1.0.0");
    ASSERT_TRUE(Versioned.hasValue());

    const auto Unversioned = registry.resolveStep("clang:compile");
    EXPECT_FALSE(Unversioned.hasValue());
}

TEST(RegistryTest, UniquePluginStepKeepsShortNameAlias)
{
    beez::core::Registry registry;

    beez::core::Step compileStep;
    compileStep.name = "compile:code";
    compileStep.phase = "build";
    compileStep.scope = "code";
    compileStep.shellRun = "echo compile";
    registry.registerPluginStep(std::move(compileStep), "coditary", "clang");

    const auto Resolved = registry.resolveStep("compile:code");
    ASSERT_TRUE(Resolved.hasValue());
    ASSERT_TRUE(Resolved.value().hasShellRun());
    EXPECT_EQ(Resolved.value().shellRun.value_or(""), "echo compile");

    const auto PluginAction = registry.resolveStep("clang:compile");
    ASSERT_TRUE(PluginAction.hasValue());
    EXPECT_EQ(PluginAction.value().shellRun.value_or(""), "echo compile");

    const auto QualifiedAction = registry.resolveStep("coditary/clang:compile");
    ASSERT_TRUE(QualifiedAction.hasValue());
    EXPECT_EQ(QualifiedAction.value().shellRun.value_or(""), "echo compile");
}
