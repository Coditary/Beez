#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/step_config.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_stage.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/contract/dsl_loader.hpp"
#include "beez/plugin/contract/executor.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include "beez/core/config/settings/run_options.hpp"
#include "beez/logging/backends/recording_logger.hpp"
#include "beez/logging/console/output_mode.hpp"

#include <atomic>
#include <chrono>
#include <thread>

#include "helpers/temp_project.hpp"
#include "helpers/test_step_config.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
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

    int execute(const std::string& command,
                const beez::core::Context& /*context*/,
                std::string* capturedOutput) override
    {
        state_->commands.push_back(command);
        ++state_->callCount;
        if (capturedOutput != nullptr)
        {
            *capturedOutput = "captured\n";
        }
        return state_->exitCode;
    }

  private:
    std::shared_ptr<ExecutorState> state_;
};

class SlowParallelTrackingExecutor : public beez::plugin::IExecutor
{
  public:
    SlowParallelTrackingExecutor(std::shared_ptr<ExecutorState> state,
                                 std::shared_ptr<std::atomic<int>> concurrent,
                                 std::shared_ptr<std::atomic<int>> peak)
        : state_(std::move(state)), concurrent_(std::move(concurrent)), peak_(std::move(peak))
    {
    }

    int execute(const std::string& command,
                const beez::core::Context& /*context*/,
                std::string* /*capturedOutput*/) override
    {
        {
            const std::scoped_lock Lock(mutex_);
            state_->commands.push_back(command);
            ++state_->callCount;
        }

        const int Current = concurrent_->fetch_add(1) + 1;
        int observed = peak_->load();
        while (Current > observed && !peak_->compare_exchange_weak(observed, Current))
        {
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        concurrent_->fetch_sub(1);
        return state_->exitCode;
    }

  private:
    std::shared_ptr<ExecutorState> state_;
    std::shared_ptr<std::atomic<int>> concurrent_;
    std::shared_ptr<std::atomic<int>> peak_;
    std::mutex mutex_;
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
        .invocation = beez::core::PhaseInvocation {.phase = "generate", .scope = "code"}});
    workflow.steps.push_back(beez::core::WorkflowStep {
        .invocation = beez::core::PhaseInvocation {.phase = "compile", .scope = "code"}});
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
    EXPECT_STREQ(beez::core::toString(beez::core::OrchestratorError::StepOrderingFailed),
                 "step ordering failed");
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

TEST(OrchestratorTest, RunStagedWorkflowTargetExecutesCumulativeStages)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step cleanStep;
    cleanStep.name = "clean-artifacts";
    cleanStep.phase = "clean";
    cleanStep.scope = "artifacts";
    cleanStep.shellRun = "echo clean";
    registry.registerStep(std::move(cleanStep));

    beez::core::Step depsStep;
    depsStep.name = "deps-install";
    depsStep.phase = "deps";
    depsStep.scope = "install";
    depsStep.shellRun = "echo deps";
    registry.registerStep(std::move(depsStep));

    beez::core::Step buildStep;
    buildStep.name = "build-backend";
    buildStep.phase = "build";
    buildStep.scope = "backend";
    buildStep.shellRun = "echo build";
    registry.registerStep(std::move(buildStep));

    beez::core::Workflow workflow;
    workflow.name = "release";
    workflow.stages = {
        beez::core::WorkflowStage {
            .name = "prepare",
            .invocations =
                {
                    beez::core::PhaseInvocation {.phase = "clean", .scope = "artifacts"},
                    beez::core::PhaseInvocation {.phase = "deps", .scope = "install"},
                },
        },
        beez::core::WorkflowStage {
            .name = "compile",
            .invocations =
                {
                    beez::core::PhaseInvocation {.phase = "build", .scope = "backend"},
                },
        },
    };
    registry.registerWorkflow(std::move(workflow));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto PrepareResult = orchestrator.run("release:prepare");
    ASSERT_TRUE(PrepareResult.hasValue());
    ASSERT_EQ(State->commands.size(), 2U);
    EXPECT_EQ(State->commands[0], "echo clean");
    EXPECT_EQ(State->commands[1], "echo deps");

    State->commands.clear();

    const auto CompileResult = orchestrator.run("release:compile");
    ASSERT_TRUE(CompileResult.hasValue());
    ASSERT_EQ(State->commands.size(), 3U);
    EXPECT_EQ(State->commands[2], "echo build");
}

TEST(OrchestratorTest, RunTaskWithoutExecutorReturnsNotAvailable)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.actions = {beez::core::makeShellAction("echo clean")};
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
    task.actions = {beez::core::makeShellAction("echo clean")};
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
    task.actions = {beez::core::makeShellAction("echo first"),
                    beez::core::makeShellAction("echo second")};
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
    task.actions = {beez::core::makeShellAction("exit 42")};
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

TEST(OrchestratorTest, RunStepPropagatesShellExecutorExitCode)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "compile";
    step.phase = "compile";
    step.scope = "code";
    step.shellRun = "exit 7";
    registry.registerStep(std::move(step));

    const auto State = std::make_shared<ExecutorState>();
    State->exitCode = 7;
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.runStep("compile");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::ExecutionFailed);
}

TEST(OrchestratorTest, RunWithCacheDisabledStillExecutesSteps)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "build";
    task.actions = {beez::core::makeShellAction("echo build")};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    const beez::core::RunOptions Options {.enableCache = false};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto Result = orchestrator.run("build");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(State->callCount, 1);
}

TEST(OrchestratorTest, RunPrefersTaskOverWorkflowWithSameName)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "build";
    task.actions = {beez::core::makeShellAction("echo task")};
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

TEST(OrchestratorTest, RunStepCallbackReceivesContextWithConfig)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "shader";
    step.phase = "generate";
    step.scope = "code";
    step.config = beez::test::makeTestConfig("shader-config");
    step.callback = [](const beez::core::Context& ctx) -> int
    {
        const auto Config = ctx.getConfig();
        if (Config == nullptr)
        {
            return 1;
        }

        const auto* typed = dynamic_cast<const beez::test::TestStepConfig*>(Config.get());
        if (typed == nullptr || typed->tag() != "shader-config")
        {
            return 1;
        }

        return 0;
    };
    registry.registerStep(std::move(step));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.runStep("shader");
    ASSERT_TRUE(Result.hasValue());
}

TEST(OrchestratorTest, RunStepCallbackRoutesLogFailureToLogger)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "lint";
    step.phase = "qa";
    step.scope = "code";
    step.callback = [](const beez::core::Context& ctx) -> int
    {
        ctx.logFailure("tidy warning\n");
        return 0;
    };
    registry.registerStep(std::move(step));

    beez::logging::RecordingLogger logger;
    const beez::core::RunOptions Options {.logger = &logger};
    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto Result = orchestrator.runStep("lint");
    ASSERT_TRUE(Result.hasValue());

    const auto HasFailure = std::ranges::any_of(
        logger.lines(),
        [](const beez::logging::RecordedLine& line)
        {
            return line.kind == beez::logging::RecordedLine::Kind::FailureOutput &&
                   line.text == "tidy warning\n";
        });
    EXPECT_TRUE(HasFailure);
}

TEST(OrchestratorTest, VerboseModeRunsStepCallbackWithoutOutputDiscard)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "noop";
    step.phase = "qa";
    step.scope = "code";
    step.callback = [](const beez::core::Context&) -> int { return 0; };
    registry.registerStep(std::move(step));

    beez::logging::RecordingLogger logger;
    const beez::core::RunOptions Options {.outputMode = beez::logging::OutputMode::Verbose,
                                          .logger = &logger};
    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto Result = orchestrator.runStep("noop");
    ASSERT_TRUE(Result.hasValue());
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

TEST(OrchestratorTest, RunTaskExecutesStepInvocation)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step compileStep;
    compileStep.name = "cpp:compile";
    compileStep.phase = "compile";
    compileStep.scope = "code";
    compileStep.shellRun = "echo compile";
    registry.registerStep(std::move(compileStep));

    beez::core::Task task;
    task.name = "full_build";
    task.actions = {beez::core::makeShellAction("echo start"),
                    beez::core::makeStepAction("cpp:compile"),
                    beez::core::makeShellAction("echo done")};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("full_build");
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 3U);
    EXPECT_EQ(State->commands[0], "echo start");
    EXPECT_EQ(State->commands[1], "echo compile");
    EXPECT_EQ(State->commands[2], "echo done");
}

TEST(OrchestratorTest, RunTaskStepInvocationReturnsNotFoundWhenStepMissing)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "full_build";
    task.actions = {beez::core::makeStepAction("missing:step")};
    registry.registerTask(std::move(task));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("full_build");
    ASSERT_FALSE(Result.hasValue());
    EXPECT_EQ(Result.error(), beez::core::OrchestratorError::NotFound);
}

TEST(OrchestratorTest, RunTaskStepInvocationMergesInlineConfig)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step shaderStep;
    shaderStep.name = "shader";
    shaderStep.phase = "generate";
    shaderStep.scope = "code";
    shaderStep.config = beez::test::makeTestConfig("base");
    shaderStep.callback = [](const beez::core::Context& ctx) -> int
    {
        const auto Config = ctx.getConfig();
        if (Config == nullptr)
        {
            return 1;
        }

        const auto* typed = dynamic_cast<const beez::test::TestStepConfig*>(Config.get());
        if (typed == nullptr || typed->tag() != "base+inline")
        {
            return 1;
        }

        return 0;
    };
    registry.registerStep(std::move(shaderStep));

    beez::core::Task task;
    task.name = "build";
    task.actions = {beez::core::makeStepAction("shader", beez::test::makeTestConfig("inline"))};
    registry.registerTask(std::move(task));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const auto Result = orchestrator.run("build");
    ASSERT_TRUE(Result.hasValue());
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

TEST(OrchestratorTest, DryRunSkipsShellExecution)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.actions = {beez::core::makeShellAction("echo clean")};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::logging::RecordingLogger logger;
    const beez::core::RunOptions Options {.dryRun = true, .logger = &logger};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto Result = orchestrator.run("clean");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(State->callCount, 0);
    ASSERT_FALSE(logger.lines().empty());
    EXPECT_EQ(logger.lines().front().kind, beez::logging::RecordedLine::Kind::BeginRun);
}

TEST(OrchestratorTest, VerboseModeCapturesShellOutput)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Task task;
    task.name = "clean";
    task.actions = {beez::core::makeShellAction("echo clean")};
    registry.registerTask(std::move(task));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::logging::RecordingLogger logger;
    const beez::core::RunOptions Options {.outputMode = beez::logging::OutputMode::Verbose,
                                          .logger = &logger};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto Result = orchestrator.run("clean");
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(State->callCount, 1);

    const auto HasCapturedOutput = std::ranges::any_of(
        logger.lines(),
        [](const beez::logging::RecordedLine& line)
        { return line.kind == beez::logging::RecordedLine::Kind::CommandOutput; });
    EXPECT_TRUE(HasCapturedOutput);
}

TEST(OrchestratorTest, RunPhaseOrdersStepsByArtifactDependencies)
{
    beez::core::Context context;
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

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "compile", .scopes = {"cpp"}};
    const auto Result = orchestrator.runPhase(Request);
    ASSERT_TRUE(Result.hasValue());
    ASSERT_EQ(State->commands.size(), 2U);
    EXPECT_EQ(State->commands[0], "echo compile");
    EXPECT_EQ(State->commands[1], "echo link");
}

TEST(OrchestratorTest, RunPhaseExecutesIndependentStepsInParallel)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step firstStep;
    firstStep.name = "check-a";
    firstStep.phase = "qa";
    firstStep.scope = "lint";
    firstStep.shellRun = "echo a";
    registry.registerStep(std::move(firstStep));

    beez::core::Step secondStep;
    secondStep.name = "check-b";
    secondStep.phase = "qa";
    secondStep.scope = "lint";
    secondStep.shellRun = "echo b";
    registry.registerStep(std::move(secondStep));

    beez::core::Step thirdStep;
    thirdStep.name = "check-c";
    thirdStep.phase = "qa";
    thirdStep.scope = "lint";
    thirdStep.shellRun = "echo c";
    registry.registerStep(std::move(thirdStep));

    const auto State = std::make_shared<ExecutorState>();
    const auto Concurrent = std::make_shared<std::atomic<int>>(0);
    const auto Peak = std::make_shared<std::atomic<int>>(0);
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<SlowParallelTrackingExecutor>(State, Concurrent, Peak));

    const beez::core::RunOptions Options {.maxThreads = 4};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const beez::core::PhaseRequest Request {.phase = "qa", .scopes = {"lint"}};
    const auto Result = orchestrator.runPhase(Request);
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(State->commands.size(), 3U);
    EXPECT_GT(Peak->load(), 1);
}

TEST(OrchestratorTest, RunPhaseSerializesMultipleCallbackStepsWithoutOrderHints)
{
    beez::core::Context context;
    beez::core::Registry registry;

    std::atomic<int> concurrentCallbacks {0};
    std::atomic<int> peakConcurrentCallbacks {0};

    const auto MakeCallbackStep = [&](const std::string& name)
    {
        beez::core::Step step;
        step.name = name;
        step.phase = "qa";
        step.scope = "code";
        step.callback = [&concurrentCallbacks,
                         &peakConcurrentCallbacks](const beez::core::Context&) -> int
        {
            const int Active = concurrentCallbacks.fetch_add(1) + 1;
            int observed = peakConcurrentCallbacks.load();
            while (Active > observed &&
                   !peakConcurrentCallbacks.compare_exchange_weak(observed, Active))
            {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            concurrentCallbacks.fetch_sub(1);
            return 0;
        };
        registry.registerStep(std::move(step));
    };

    MakeCallbackStep("callback-a");
    MakeCallbackStep("callback-b");
    MakeCallbackStep("callback-c");

    beez::plugin::PluginHost pluginHost;
    const beez::core::RunOptions Options {.maxThreads = 4};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const beez::core::PhaseRequest Request {.phase = "qa", .scopes = {"code"}};
    const auto Result = orchestrator.runPhase(Request);
    ASSERT_TRUE(Result.hasValue());
    EXPECT_EQ(peakConcurrentCallbacks.load(), 1);
}

namespace
{

void writeProjectFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(OrchestratorTest, SkipsCachedStepOnSecondRun)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    beez::core::Context context(Project.path());
    beez::core::Registry registry;

    int callbackCount = 0;
    beez::core::Step compileStep;
    compileStep.name = "compile";
    compileStep.phase = "compile";
    compileStep.scope = "cpp";
    compileStep.input = {"src/**/*.cpp"};
    compileStep.output = {"build/**/*.o"};
    compileStep.callback = [&Project, &callbackCount](const beez::core::Context&)
    {
        ++callbackCount;
        writeProjectFile(Project.path() / "build" / "main.o", "object\n");
        return 0;
    };
    registry.registerStep(std::move(compileStep));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "compile", .scopes = {"cpp"}};
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    EXPECT_EQ(callbackCount, 1);
}

TEST(OrchestratorTest, ReexecutesWhenCachedOutputsAreMissing)
{
    const beez::test::TempProject Project;
    writeProjectFile(Project.path() / "src" / "main.cpp", "int main() {}\n");

    beez::core::Context context(Project.path());
    beez::core::Registry registry;

    int callbackCount = 0;
    beez::core::Step compileStep;
    compileStep.name = "compile";
    compileStep.phase = "compile";
    compileStep.scope = "cpp";
    compileStep.input = {"src/**/*.cpp"};
    compileStep.output = {"build/**/*.o"};
    compileStep.callback = [&Project, &callbackCount](const beez::core::Context&)
    {
        ++callbackCount;
        writeProjectFile(Project.path() / "build" / "main.o", "object\n");
        return 0;
    };
    registry.registerStep(std::move(compileStep));

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "compile", .scopes = {"cpp"}};
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    std::filesystem::remove(Project.path() / "build" / "main.o");
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    EXPECT_EQ(callbackCount, 2);
}

TEST(OrchestratorTest, StepsWithoutArtifactsAlwaysExecute)
{
    beez::core::Context context;
    beez::core::Registry registry;

    beez::core::Step step;
    step.name = "noop";
    step.phase = "generate";
    step.scope = "docs";
    step.shellRun = "echo noop";
    registry.registerStep(std::move(step));

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));

    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    const beez::core::PhaseRequest Request {.phase = "generate", .scopes = {"docs"}};
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    ASSERT_TRUE(orchestrator.runPhase(Request).hasValue());
    EXPECT_EQ(State->callCount, 2);
}
