#include "beez/core/orchestrator.h"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/plugin/plugin_host.h"
#include "beez/version.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::optional<beez::core::Workflow> requireWorkflow(const beez::core::Registry& registry,
                                                    const std::string& name)
{
    const auto Found = registry.findWorkflow(name);
    EXPECT_TRUE(Found.has_value());
    return Found;
}

void expectSequentialStep(const beez::core::WorkflowStep& step,
                          const std::string& phase,
                          const std::string& scope)
{
    ASSERT_FALSE(step.isParallel());
    ASSERT_EQ(step.invocations.size(), 1U);
    EXPECT_EQ(step.invocations[0].phase, phase);
    EXPECT_EQ(step.invocations[0].scope, scope);
}

void expectParallelStep(const beez::core::WorkflowStep& step,
                        const std::vector<std::pair<std::string, std::string>>& phases)
{
    ASSERT_TRUE(step.isParallel());
    ASSERT_EQ(step.invocations.size(), phases.size());
    for (std::size_t index = 0; index < phases.size(); ++index)
    {
        EXPECT_EQ(step.invocations[index].phase, phases[index].first);
        EXPECT_EQ(step.invocations[index].scope, phases[index].second);
    }
}

}  // namespace

TEST(VersionTest, MajorVersion)
{
    EXPECT_EQ(beez::version::MajorVersion, 0);
}

TEST(VersionTest, MinorVersion)
{
    EXPECT_EQ(beez::version::MinorVersion, 1);
}

TEST(VersionTest, PatchVersion)
{
    EXPECT_EQ(beez::version::PatchVersion, 0);
}

TEST(VersionTest, VersionString)
{
    EXPECT_STREQ(beez::version::VersionString, "0.1.0");
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
    const auto& foundTask = *Found;
    EXPECT_EQ(foundTask.name, "clean");
    EXPECT_EQ(foundTask.run, "rm -fr app.o");
    EXPECT_TRUE(foundTask.isOrphan());
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
    const auto& foundTask = *Found;
    EXPECT_EQ(foundTask.phase, std::optional<std::string> {"generate"});
    EXPECT_EQ(foundTask.scope, std::optional<std::string> {"docs"});
    EXPECT_FALSE(foundTask.isOrphan());
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

    const auto FoundWorkflow = requireWorkflow(registry, "build");
    ASSERT_TRUE(FoundWorkflow.has_value());
    if (!FoundWorkflow)
    {
        return;
    }
    const auto& storedWorkflow = *FoundWorkflow;
    ASSERT_EQ(storedWorkflow.steps.size(), 2U);
    expectSequentialStep(storedWorkflow.steps[0], "generate", "code");
    expectSequentialStep(storedWorkflow.steps[1], "compile", "code");
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

    const auto FoundWorkflow = requireWorkflow(registry, "ci");
    ASSERT_TRUE(FoundWorkflow.has_value());
    if (!FoundWorkflow)
    {
        return;
    }
    const auto& storedWorkflow = *FoundWorkflow;
    ASSERT_EQ(storedWorkflow.steps.size(), 2U);
    expectParallelStep(storedWorkflow.steps[0], {{"generate", "docs"}, {"generate", "code"}});
    expectSequentialStep(storedWorkflow.steps[1], "compile", "code");
}

TEST(OrchestratorTest, RunUnknownNameReturnsNotFound)
{
    beez::core::Context context;
    beez::core::Registry registry;
    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("missing");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::NotFound);
}

TEST(OrchestratorTest, RunWorkflowReturnsNotImplemented)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Workflow workflow;
    workflow.name = "build";
    registry.registerWorkflow(std::move(workflow));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("build");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::WorkflowNotImplemented);
}
