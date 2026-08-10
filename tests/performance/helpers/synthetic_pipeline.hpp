#pragma once

#include "artifact_vault.hpp"

#include "beez/core/model/step.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace beez::perf
{

struct PipelineScenario
{
    std::string name;
    std::size_t fileCount = 0;
    std::size_t stepCount = 0;
    std::size_t shardCount = 1;
    std::size_t laneCount = 1;
    std::size_t materializeLimit = 512;
};

struct PipelineBuildResult
{
    std::vector<core::Step> steps;
    ArtifactVault vault;
};

inline PipelineBuildResult buildPipeline(const std::filesystem::path& workspace,
                                         const PipelineScenario& scenario)
{
    const ArtifactVaultSpec VaultSpec {.fileCount = scenario.fileCount,
                                       .shardCount = std::max<std::size_t>(scenario.shardCount, 1U),
                                       .laneCount = std::max<std::size_t>(scenario.laneCount, 1U),
                                       .materializeLimit = scenario.materializeLimit};

    PipelineBuildResult result {.steps = {}, .vault = ArtifactVault(workspace, VaultSpec)};

    static const std::vector<std::string> StageExtensions = {
        "raw", "lint", "fmt", "obj", "pak", "tex"};

    result.steps.reserve(scenario.stepCount);
    for (std::size_t index = 0; index < scenario.stepCount; ++index)
    {
        core::Step step;
        step.name = "vault:" + scenario.name + ":" + std::to_string(index);
        step.phase = "stress";
        step.scope = "vault";
        step.shellRun = "echo " + step.name;

        const std::size_t Shard = index % VaultSpec.shardCount;
        const std::size_t Lane = (index / 3U) % VaultSpec.laneCount;
        const std::size_t Stage = index % StageExtensions.size();
        const std::size_t NextStage = (Stage + 1U) % StageExtensions.size();
        const std::size_t BridgeShard = (Shard + 1U) % VaultSpec.shardCount;
        const std::size_t InputPass = index;
        const std::size_t OutputPass = index + 1U;

        if (index % 11U == 5U)
        {
            step.mutate = {
                result.vault.globForPass(Shard, Lane, InputPass, StageExtensions[Stage])};
        }
        else if (index % 13U == 0U && scenario.stepCount > 1U)
        {
            step.input = {
                result.vault.globForPass(BridgeShard, Lane, InputPass, StageExtensions[Stage])};
            step.output = {
                result.vault.globForPass(Shard, Lane, OutputPass, StageExtensions[NextStage])};
        }
        else
        {
            step.input = {result.vault.globForPass(Shard, Lane, InputPass, StageExtensions[Stage])};
            step.output = {
                result.vault.globForPass(Shard, Lane, OutputPass, StageExtensions[NextStage])};
        }

        result.steps.push_back(std::move(step));
    }

    return result;
}

}  // namespace beez::perf
