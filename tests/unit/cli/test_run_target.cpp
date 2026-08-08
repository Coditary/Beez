#include "beez/cli/run_target.hpp"

#include "beez/cli/parsed_options.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/plugin/plugin_host.h"

#include <gtest/gtest.h>

#include <string>

TEST(RunTargetTest, MisspelledTargetPrintsDidYouMean)
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
    const int ExitCode = beez::cli::runParsedInvocation(orchestrator, registry, options);
    const auto Stderr = testing::internal::GetCapturedStderr();
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(ExitCode, 1);
    EXPECT_NE(Stderr.find("Did you mean 'build'?"), std::string::npos);
}
