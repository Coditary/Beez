#pragma once

#include "beez/core/registry.h"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"

#include "test_step_config.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace beez::test
{

inline std::optional<core::Workflow> requireWorkflow(const core::Registry& registry,
                                                     const std::string& name)
{
    const auto Found = registry.findWorkflow(name);
    EXPECT_TRUE(Found.has_value());
    return Found;
}

inline void expectSequentialStep(const core::WorkflowStep& step,
                                 const std::string& phase,
                                 const std::string& scope)
{
    ASSERT_FALSE(step.isParallel());
    ASSERT_EQ(step.invocations.size(), 1U);
    EXPECT_EQ(step.invocations[0].phase, phase);
    EXPECT_EQ(step.invocations[0].scope, scope);
}

inline void expectParallelStep(const core::WorkflowStep& step,
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

inline void expectStepConfigTag(const core::Registry& registry,
                                const std::string& name,
                                const std::string& expectedTag)
{
    const auto Found = registry.findStep(name);
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_TRUE(Found->hasConfig());
    const auto* stepConfig = dynamic_cast<const TestStepConfig*>(Found->config.get());
    ASSERT_NE(stepConfig, nullptr);
    EXPECT_EQ(stepConfig->tag(), expectedTag);
}

inline void expectStepHasConfig(const core::Registry& registry, const std::string& name)
{
    const auto Found = registry.findStep(name);
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_TRUE(Found->hasConfig());
}

}  // namespace beez::test
