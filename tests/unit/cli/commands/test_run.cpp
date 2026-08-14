#include "beez/cli/commands/run.hpp"

#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/model/phase_request.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/orchestrator/orchestrator.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/plugin/contract/executor.hpp"
#include "beez/plugin/host/plugin_host.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace
{

struct ExecutorState
{
    int exitCode = 0;
};

class RecordingExecutor : public beez::plugin::IExecutor
{
  public:
    explicit RecordingExecutor(std::shared_ptr<ExecutorState> state) : state_(std::move(state)) {}

    int execute(const std::string& /*command*/,
                const beez::core::Context& /*context*/,
                std::string* /*capturedOutput*/) override
    {
        return state_->exitCode;
    }

  private:
    std::shared_ptr<ExecutorState> state_;
};

}  // namespace

TEST(RunCommandTest, SilentModeSuppressesDidYouMean)
{
    beez::core::Context context;
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {.name = "build"});

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    options.target = "biuld";

    testing::internal::CaptureStderr();
    const int ExitCode = beez::cli::runOrchestratorCommand(
        orchestrator, registry, context, options, beez::logging::OutputMode::Silent);
    const auto Stderr = testing::internal::GetCapturedStderr();

    EXPECT_EQ(ExitCode, 1);
    EXPECT_TRUE(Stderr.empty());
}

TEST(RunCommandTest, MisspelledTargetPrintsDidYouMean)
{
    beez::core::Context context;
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {.name = "build"});

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    options.target = "biuld";

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);
    const auto Stderr = testing::internal::GetCapturedStderr();
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(ExitCode, 1);
    EXPECT_NE(Stderr.find("Did you mean 'build'?"), std::string::npos);
}

TEST(RunCommandTest, MissingTargetReturnsFailure)
{
    beez::core::Context context;
    beez::core::Registry registry;
    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);

    EXPECT_EQ(ExitCode, 1);
}

TEST(RunCommandTest, SuccessfulTaskReturnsTaskExitCode)
{
    beez::core::Context context;
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {
        .name = "build",
        .actions = {beez::core::makeShellAction("echo ok")},
    });

    const auto State = std::make_shared<ExecutorState>();
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    options.target = "build";

    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);

    EXPECT_EQ(ExitCode, 0);
}

TEST(RunCommandTest, PhaseRequestRunsPhase)
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

    beez::cli::ParsedOptions options;
    options.phaseRequest = beez::core::PhaseRequest {.phase = "compile", .scopes = {"code"}};

    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);

    EXPECT_EQ(ExitCode, 0);
}

TEST(RunCommandTest, StepNameRunsRegisteredStep)
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

    beez::cli::ParsedOptions options;
    options.stepName = "compile";

    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);

    EXPECT_EQ(ExitCode, 0);
}

TEST(RunCommandTest, AmbiguousStepPrintsQualifiedCandidates)
{
    beez::core::Context context;
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

    beez::plugin::PluginHost pluginHost;
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    options.stepName = "check";

    testing::internal::CaptureStderr();
    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);
    const auto Stderr = testing::internal::GetCapturedStderr();

    EXPECT_EQ(ExitCode, 1);
    EXPECT_NE(Stderr.find("ambiguous"), std::string::npos);
    EXPECT_NE(Stderr.find("clang-tidy"), std::string::npos);
    EXPECT_NE(Stderr.find("cppcheck"), std::string::npos);
}

TEST(RunCommandTest, ExecutionFailurePrintsOrchestratorError)
{
    beez::core::Context context;
    beez::core::Registry registry;
    registry.registerTask(beez::core::Task {
        .name = "build",
        .actions = {beez::core::makeShellAction("exit 1")},
    });

    const auto State = std::make_shared<ExecutorState>();
    State->exitCode = 1;
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>(State));
    beez::core::Orchestrator orchestrator(registry, context, pluginHost);

    beez::cli::ParsedOptions options;
    options.target = "build";

    testing::internal::CaptureStderr();
    const int ExitCode =
        beez::cli::runOrchestratorCommand(orchestrator, registry, context, options);
    const auto Stderr = testing::internal::GetCapturedStderr();

    EXPECT_EQ(ExitCode, 1);
    EXPECT_NE(Stderr.find("Error:"), std::string::npos);
}
