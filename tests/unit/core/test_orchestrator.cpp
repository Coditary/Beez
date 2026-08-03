#include "beez/core/context.h"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/plugin/dsl_loader.hpp"
#include "beez/plugin/executor.hpp"
#include "beez/plugin/plugin_host.h"

#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace
{

struct ExecutorState
{
    std::string lastCommand;
    int exitCode = 0;
    int callCount = 0;
};

class RecordingExecutor : public beez::plugin::IExecutor
{
  public:
    explicit RecordingExecutor(std::shared_ptr<ExecutorState> state) : state_(std::move(state)) {}

    int execute(const std::string& command, const beez::core::Context& /*context*/) override
    {
        state_->lastCommand = command;
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

}  // namespace

TEST(OrchestratorErrorTest, ToStringCoversAllErrors)
{
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::NotFound),
                 "name not found in registry");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::WorkflowNotImplemented),
                 "workflow execution is not yet implemented");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::ExecutionFailed),
                 "task execution failed");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::BuildScriptNotFound),
                 "build.lua not found");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::BuildScriptLoadFailed),
                 "failed to load build.lua");
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::ExecutorNotAvailable),
                 "no shell executor plugin available");
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

TEST(OrchestratorTest, RunTaskWithoutExecutorReturnsNotAvailable)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.run = "echo clean";
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
    task.run = "echo clean";
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("clean");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(Result.value(), 0);
    EXPECT_EQ(State->lastCommand, "echo clean");
    EXPECT_EQ(State->callCount, 1);
}

TEST(OrchestratorTest, RunTaskPropagatesExecutorExitCode)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "fail";
    task.run = "exit 42";
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    State->exitCode = 42;
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("fail");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(Result.value(), 42);
}

TEST(OrchestratorTest, RunPrefersTaskOverWorkflowWithSameName)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "build";
    task.run = "echo task";
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
    EXPECT_EQ(State->lastCommand, "echo task");
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
