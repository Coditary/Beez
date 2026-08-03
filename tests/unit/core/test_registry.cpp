#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"

#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>

TEST(RegistryTest, FindUnknownTaskReturnsEmpty)
{
    const beez::core::Registry Registry;
    EXPECT_FALSE(Registry.findTask("missing").has_value());
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
    task.run = "rm -fr app.o";
    registry.registerTask(std::move(task));

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->name, "clean");
    EXPECT_EQ(Found->run, "rm -fr app.o");
    EXPECT_TRUE(Found->isOrphan());
}

TEST(RegistryTest, RegisterAndFindPhaseBoundTask)
{
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "doxygen";
    task.run = "doxygen Doxyfile";
    task.phase = "generate";
    task.scope = "docs";
    registry.registerTask(std::move(task));

    const auto Found = registry.findTask("doxygen");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->phase, std::optional<std::string> {"generate"});
    EXPECT_EQ(Found->scope, std::optional<std::string> {"docs"});
    EXPECT_FALSE(Found->isOrphan());
}

TEST(RegistryTest, RegisterTaskOverwritesExisting)
{
    beez::core::Registry registry;

    beez::core::Task first;
    first.name = "clean";
    first.run = "rm -fr app.o";
    registry.registerTask(std::move(first));

    beez::core::Task second;
    second.name = "clean";
    second.run = "echo updated";
    registry.registerTask(std::move(second));

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->run, "echo updated");
}

TEST(RegistryTest, TasksForPhaseFiltersByPhaseAndScope)
{
    beez::core::Registry registry;

    beez::core::Task docsTask;
    docsTask.name = "doxygen";
    docsTask.run = "doxygen";
    docsTask.phase = "generate";
    docsTask.scope = "docs";
    registry.registerTask(std::move(docsTask));

    beez::core::Task codeTask;
    codeTask.name = "protobuf";
    codeTask.run = "protoc";
    codeTask.phase = "generate";
    codeTask.scope = "code";
    registry.registerTask(std::move(codeTask));

    const auto DocsMatches = registry.tasksForPhase("generate", "docs");
    ASSERT_EQ(DocsMatches.size(), 1U);
    EXPECT_EQ(DocsMatches.front().name, "doxygen");

    const auto CodeMatches = registry.tasksForPhase("generate", "code");
    ASSERT_EQ(CodeMatches.size(), 1U);
    EXPECT_EQ(CodeMatches.front().name, "protobuf");
}

TEST(RegistryTest, TasksForPhaseReturnsEmptyWhenNoMatch)
{
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "compile";
    task.run = "make";
    task.phase = "compile";
    task.scope = "code";
    registry.registerTask(std::move(task));

    EXPECT_TRUE(registry.tasksForPhase("generate", "docs").empty());
    EXPECT_TRUE(registry.tasksForPhase("compile", "docs").empty());
}

TEST(RegistryTest, TasksForPhaseWildcardScopeMatchesAllScopes)
{
    beez::core::Registry registry;

    beez::core::Task docsTask;
    docsTask.name = "doxygen";
    docsTask.run = "doxygen";
    docsTask.phase = "generate";
    docsTask.scope = "docs";
    registry.registerTask(std::move(docsTask));

    beez::core::Task codeTask;
    codeTask.name = "protobuf";
    codeTask.run = "protoc";
    codeTask.phase = "generate";
    codeTask.scope = "code";
    registry.registerTask(std::move(codeTask));

    const auto Matches = registry.tasksForPhase("generate", "*");
    ASSERT_EQ(Matches.size(), 2U);
}

TEST(RegistryTest, OrphanTasksAreExcludedFromPhaseQuery)
{
    beez::core::Registry registry;

    beez::core::Task orphan;
    orphan.name = "clean";
    orphan.run = "rm -fr app.o";
    registry.registerTask(std::move(orphan));

    EXPECT_TRUE(registry.tasksForPhase("generate", "*").empty());
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
