#include "beez/core/glob_pattern.hpp"
#include "beez/core/orchestrator.h"
#include "beez/core/phase_request.hpp"
#include "beez/core/registry.h"
#include "beez/core/run_options.hpp"
#include "beez/core/step_order.hpp"
#include "beez/plugin/executor.hpp"
#include "beez/plugin/plugin_host.h"

#include "helpers/artifact_vault.hpp"
#include "helpers/performance_workspace.hpp"
#include "helpers/synthetic_pipeline.hpp"
#include "helpers/timing_report.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

class PerformanceReportEnvironment final : public ::testing::Environment
{
  public:
    void TearDown() override
    {
        beez::perf::TimingReport::instance().write(beez::perf::performanceReportPath());
    }
};

const auto* const PerformanceReport =
    ::testing::AddGlobalTestEnvironment(new PerformanceReportEnvironment());

class RecordingExecutor final : public beez::plugin::IExecutor
{
  public:
    int execute(const std::string& /*command*/,
                const beez::core::Context& /*context*/,
                std::string* /*capturedOutput*/) override
    {
        return 0;
    }
};

void expectOrderingIsValid(const std::vector<beez::core::Step>& steps,
                           const std::vector<beez::core::Step>& ordered,
                           const beez::core::IGlobMatcher& matcher)
{
    std::unordered_map<std::string, std::size_t> positions;
    positions.reserve(ordered.size());
    for (std::size_t index = 0; index < ordered.size(); ++index)
    {
        positions.emplace(ordered[index].name, index);
    }

    for (const auto& producer : steps)
    {
        const auto ProducerPosition = positions.at(producer.name);
        for (const auto& consumer : steps)
        {
            if (producer.name == consumer.name)
            {
                continue;
            }

            bool producerBeforeConsumer = false;
            for (const auto& outputPattern : producer.output)
            {
                for (const auto& inputPattern : consumer.input)
                {
                    if (matcher.patternsOverlap(outputPattern, inputPattern))
                    {
                        producerBeforeConsumer = true;
                    }
                }
            }

            for (const auto& mutatePattern : producer.mutate)
            {
                for (const auto& inputPattern : consumer.input)
                {
                    if (matcher.patternsOverlap(mutatePattern, inputPattern))
                    {
                        producerBeforeConsumer = true;
                    }
                }
            }

            if (producerBeforeConsumer)
            {
                ASSERT_LT(ProducerPosition, positions.at(consumer.name));
            }
        }
    }
}

void validateOrdering(const std::vector<beez::core::Step>& steps,
                      const std::vector<beez::core::Step>& ordered,
                      const beez::core::IGlobMatcher& matcher,
                      std::size_t stepCount)
{
    ASSERT_EQ(ordered.size(), stepCount);

    // Full pairwise validation is O(steps²) — keep it for smaller scenarios only.
    if (stepCount <= 500)
    {
        expectOrderingIsValid(steps, ordered, matcher);
    }
}

void runThroughputScenario(const beez::perf::PipelineScenario& scenario)
{
    beez::perf::PerformanceWorkspace workspace(scenario.name);
    auto pipeline = beez::perf::buildPipeline(workspace.path(), scenario);
    beez::core::IGlobMatcher& matcher = beez::core::defaultGlobMatcher();

    const auto OrderStart = std::chrono::steady_clock::now();
    const auto Ordered = beez::core::orderSteps(pipeline.steps, {}, matcher);
    const auto OrderEnd = std::chrono::steady_clock::now();

    ASSERT_TRUE(Ordered.hasValue());
    validateOrdering(pipeline.steps, Ordered.value(), matcher, scenario.stepCount);

    beez::core::Registry registry;
    for (auto step : pipeline.steps)
    {
        registry.registerStep(std::move(step));
    }

    const auto RegistryStart = std::chrono::steady_clock::now();
    const auto RegistryOrdered = registry.stepsForPhase("stress", "vault");
    const auto RegistryEnd = std::chrono::steady_clock::now();

    ASSERT_TRUE(RegistryOrdered.hasValue());
    ASSERT_EQ(RegistryOrdered.value().size(), scenario.stepCount);

    beez::core::Context context(workspace.path());
    beez::plugin::PluginHost pluginHost;
    pluginHost.setExecutor(std::make_unique<RecordingExecutor>());
    const beez::core::RunOptions Options {.dryRun = true};
    beez::core::Orchestrator orchestrator(registry, context, pluginHost, Options);

    const auto OrchestratorStart = std::chrono::steady_clock::now();
    const auto PhaseResult = orchestrator.runPhase({.phase = "stress", .scopes = {"vault"}});
    const auto OrchestratorEnd = std::chrono::steady_clock::now();

    ASSERT_TRUE(PhaseResult.hasValue());

    beez::perf::TimingReport::instance().record(
        {.scenario = scenario.name,
         .suite = "artifact-vault",
         .virtualFiles = pipeline.vault.virtualFileCount(),
         .materializedFiles = pipeline.vault.materializedFileCount(),
         .steps = scenario.stepCount,
         .orderMs = beez::perf::elapsedMs(OrderStart, OrderEnd),
         .registryMs = beez::perf::elapsedMs(RegistryStart, RegistryEnd),
         .orchestratorMs = beez::perf::elapsedMs(OrchestratorStart, OrchestratorEnd)});
}

}  // namespace

TEST(ArtifactVaultThroughputTest, SparkTenByTen)
{
    runThroughputScenario({.name = "spark",
                           .fileCount = 10,
                           .stepCount = 10,
                           .shardCount = 2,
                           .laneCount = 2,
                           .materializeLimit = 10});
}

TEST(ArtifactVaultThroughputTest, BrookHundredByHundred)
{
    runThroughputScenario({.name = "brook",
                           .fileCount = 100,
                           .stepCount = 100,
                           .shardCount = 10,
                           .laneCount = 5,
                           .materializeLimit = 100});
}

TEST(ArtifactVaultThroughputTest, DeltaThousandStepsTenFiles)
{
    runThroughputScenario({.name = "delta",
                           .fileCount = 10,
                           .stepCount = 1000,
                           .shardCount = 2,
                           .laneCount = 2,
                           .materializeLimit = 10});
}

TEST(ArtifactVaultThroughputTest, RiverThousandFilesHundredSteps)
{
    runThroughputScenario({.name = "river",
                           .fileCount = 1000,
                           .stepCount = 100,
                           .shardCount = 20,
                           .laneCount = 5,
                           .materializeLimit = 512});
}

TEST(ArtifactVaultThroughputTest, MesaHundredThousandFilesTenSteps)
{
    runThroughputScenario({.name = "mesa",
                           .fileCount = 100000,
                           .stepCount = 10,
                           .shardCount = 100,
                           .laneCount = 10,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, HorizonHundredThousandFilesHundredSteps)
{
    runThroughputScenario({.name = "horizon",
                           .fileCount = 100000,
                           .stepCount = 100,
                           .shardCount = 100,
                           .laneCount = 10,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, StormThousandByThousand)
{
    runThroughputScenario({.name = "storm",
                           .fileCount = 1000,
                           .stepCount = 1000,
                           .shardCount = 40,
                           .laneCount = 10,
                           .materializeLimit = 512});
}

TEST(ArtifactVaultThroughputTest, TorrentFiveThousandSteps)
{
    runThroughputScenario({.name = "torrent",
                           .fileCount = 500,
                           .stepCount = 5000,
                           .shardCount = 50,
                           .laneCount = 10,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, CycloneTenThousandSteps)
{
    runThroughputScenario({.name = "cyclone",
                           .fileCount = 1000,
                           .stepCount = 10000,
                           .shardCount = 100,
                           .laneCount = 20,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, SuperstormFiveThousandByFiveThousand)
{
    runThroughputScenario({.name = "superstorm",
                           .fileCount = 5000,
                           .stepCount = 5000,
                           .shardCount = 100,
                           .laneCount = 25,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, NebulaFiveHundredThousandFilesFiveHundredSteps)
{
    runThroughputScenario({.name = "nebula",
                           .fileCount = 500000,
                           .stepCount = 500,
                           .shardCount = 200,
                           .laneCount = 25,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, AbyssOneMillionFilesFiftySteps)
{
    runThroughputScenario({.name = "abyss",
                           .fileCount = 1000000,
                           .stepCount = 50,
                           .shardCount = 250,
                           .laneCount = 20,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, ColossusTenThousandByTenThousand)
{
    runThroughputScenario({.name = "colossus",
                           .fileCount = 10000,
                           .stepCount = 10000,
                           .shardCount = 200,
                           .laneCount = 50,
                           .materializeLimit = 0});
}

TEST(ArtifactVaultThroughputTest, TitanOneMillionFilesTenThousandSteps)
{
    runThroughputScenario({.name = "titan",
                           .fileCount = 1000000,
                           .stepCount = 10000,
                           .shardCount = 250,
                           .laneCount = 50,
                           .materializeLimit = 0});
}
