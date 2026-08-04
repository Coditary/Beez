#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/phase_request.hpp"
#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/plugin/dsl_loader.hpp"
#include "beez/plugin/executor.hpp"
#include "beez/plugin/plugin_host.h"

#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct ExecutorState
{
    std::vector<std::string> commands;
    int exitCode = 0;
    int callCount = 0;
};

class RecordingExecutor : public beez::plugin::IExecutor
{
  public:
    explicit RecordingExecutor(std::shared_ptr<ExecutorState> state) : state_(std::move(state)) {}

    int execute(const std::string& command, const beez::core::Context& /*context*/) override
    {
        state_->commands.push_back(command);
        ++state_->callCount;
        return state_->exitCode;
    }

  private:
    std::shared_ptr<ExecutorState> state_;
};

struct DslLoaderState
{
    bool loadResult = true;
    bool loadCalled = false;
};

class RecordingDslLoader : public beez::plugin::IDslLoader
{
  public:
    explicit RecordingDslLoader(std::shared_ptr<DslLoaderState> state) : state_(std::move(state)) {}

    bool load(const beez::core::Context& /*context*/, beez::core::Registry& /*registry*/) override
    {
        state_->loadCalled = true;
        return state_->loadResult;
    }

  private:
    std::shared_ptr<DslLoaderState> state_;
};

void registerBuildWorkflow(beez::core::Registry& registry)
{
    beez::core::Step generateStep;
    generateStep.name = "gen-code";
    generateStep.phase = "generate";
    generateStep.scope = "code";
    generateStep.shellRun = "echo generate";
    registry.registerStep(std::move(generateStep));

    beez::core::Step compileStep;
    compileStep.name = "compile";
    compileStep.phase = "compile";
    compileStep.scope = "code";
    compileStep.shellRun = "echo compile";
    registry.registerStep(std::move(compileStep));

    beez::core::Workflow workflow;
    workflow.name = "build";
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}}});
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocations = {beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}}});
    registry.registerWorkflow(std::move(workflow));
}

}  // namespace

TEST(OrchestratorErrorTest, ToStringCoversAllErrors)
{
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::NotFound),
                 "name not found in registry");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::ExecutionFailed),
                 "task execution failed");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::BuildScriptNotFound),
                 "build.lua not found");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::BuildScriptLoadFailed),
                 "failed to load build.lua");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::ExecutorNotAvailable),
                 "no shell executor plugin available");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::InvalidPhaseRequest),
                 "invalid phase request");
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

TEST(OrchestratorTest, RunWorkflowExecutesPhaseStepsInOrder)
{
    beez::core::Context context;
    beez::core::Registry registry;
    registerBuildWorkflow(registry);

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("build");
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 2U);
    EXPECT_EQ(State->commands[0], "echo generate");
    EXPECT_EQ(State->commands[1], "echo compile");
}

TEST(OrchestratorTest, RunTaskWithoutExecutorReturnsNotAvailable)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.commands = {"echo clean"};
    registry.registerTask(std::move(task));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("clean");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::ExecutorNotAvailable);
}

TEST(OrchestratorTest, RunTaskExecutesViaRegisteredExecutor)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.commands = {"echo clean"};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("clean");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(Result.value(), 0);
    ASSERT_EQ(State->commands.size(), 1U);
    EXPECT_EQ(State->commands[0], "echo clean");
    EXPECT_EQ(State->callCount, 1);
}

TEST(OrchestratorTest, RunTaskExecutesMultipleCommandsSequentially)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "hello";
    task.commands = {"echo first", "echo second"};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("hello");
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 2U);
    EXPECT_EQ(State->commands[0], "echo first");
    EXPECT_EQ(State->commands[1], "echo second");
}

TEST(OrchestratorTest, RunTaskPropagatesExecutorExitCode)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "fail";
    task.commands = {"exit 42"};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    State->exitCode = 42;
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("fail");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::ExecutionFailed);
}

TEST(OrchestratorTest, RunPrefersTaskOverWorkflowWithSameName)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "build";
    task.commands = {"echo task"};
    registry.registerTask(std::move(task));

    beez::core::Workflow workflow;
    workflow.name = "build";
    registry.registerWorkflow(std::move(workflow));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("build");
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 1U);
    EXPECT_EQ(State->commands[0], "echo task");
}

TEST(OrchestratorTest, RunStepExecutesRegisteredStep)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "compile";
    step.phase = "compile";
    step.scope = "code";
    step.shellRun = "echo compile";
    registry.registerStep(std::move(step));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.runStep("compile");
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 1U);
    EXPECT_EQ(State->commands[0], "echo compile");
}

TEST(OrchestratorTest, RunPhaseExecutesAllScopesWhenNoneSpecified)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step docsStep;
    docsStep.name = "docs";
    docsStep.phase = "generate";
    docsStep.scope = "docs";
    docsStep.shellRun = "echo docs";
    registry.registerStep(std::move(docsStep));

    beez::core::Step codeStep;
    codeStep.name = "code";
    codeStep.phase = "generate";
    codeStep.scope = "code";
    codeStep.shellRun = "echo code";
    registry.registerStep(std::move(codeStep));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "generate"};
    const auto Result = orchestrator.runPhase(Request);
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(State->callCount, 2);
}

TEST(OrchestratorTest, RunPhaseExecutesOnlyRequestedScopes)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step docsStep;
    docsStep.name = "docs";
    docsStep.phase = "generate";
    docsStep.scope = "docs";
    docsStep.shellRun = "echo docs";
    registry.registerStep(std::move(docsStep));

    beez::core::Step codeStep;
    codeStep.name = "code";
    codeStep.phase = "generate";
    codeStep.scope = "code";
    codeStep.shellRun = "echo code";
    registry.registerStep(std::move(codeStep));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "generate", .scopes = {"code"}};
    const auto Result = orchestrator.runPhase(Request);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 1U);
    EXPECT_EQ(State->commands[0], "echo code");
}

TEST(OrchestratorTest, LoadBuildScriptReturnsNotFoundWhenMissing)
{
    const beez::test::TempProject Project;
    beez::core::Context context(Project.path());
    beez::core::Registry registry;
    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.loadBuildScript();
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::BuildScriptNotFound);
}

TEST(OrchestratorTest, LoadBuildScriptReturnsLoadFailedWhenLoaderFails)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"clean\", \"echo clean\")\n");

    beez::core::Context context(Project.path());
    beez::core::Registry registry;

    const auto State = std::make_shared<DslLoaderState>();
    State->loadResult = false;
    beez::plugin::PluginHost pluginHost;
    pluginHost.setDslLoader(std::make_unique<RecordingDslLoader>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.loadBuildScript();
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::BuildScriptLoadFailed);
    EXPECT_TRUE(State->loadCalled);
}

TEST(OrchestratorTest, LoadBuildScriptSucceedsWhenLoaderSucceeds)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"clean\", \"echo clean\")\n");

    beez::core::Context context(Project.path());
    beez::core::Registry registry;

    const auto State = std::make_shared<DslLoaderState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setDslLoader(std::make_unique<RecordingDslLoader>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.loadBuildScript();
    ASSERT_TRUE(Result.hasValue());
    EXPECT_TRUE(State->loadCalled);
}
