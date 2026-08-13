#pragma once

#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/core/model/workflow.hpp"
#include "beez/core/model/workflow_step.hpp"
#include "beez/core/registry/registry.hpp"

#include "test_step_config.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace beez::test
{

inline std::optional<core::Task> requireTask(const core::Registry& registry,
                                             const std::string& name)
{
    const auto Found = registry.findTask(name);
    EXPECT_TRUE(Found.has_value());
    return Found;
}

inline const core::TaskShellAction* shellActionAt(const core::Task& task, std::size_t index)
{
    if (index >= task.actions.size())
    {
        ADD_FAILURE() << "task action index out of range: " << index;
        return nullptr;
    }

    return std::get_if<core::TaskShellAction>(task.actions.data() + index);
}

inline const core::TaskShellAction* shellActionAt(const std::optional<core::Task>& task,
                                                  std::size_t index)
{
    if (!task.has_value())
    {
        ADD_FAILURE() << "task not found";
        return nullptr;
    }
    return shellActionAt(*task, index);
}

inline const core::TaskStepAction* stepActionAt(const core::Task& task, std::size_t index)
{
    if (index >= task.actions.size())
    {
        ADD_FAILURE() << "task action index out of range: " << index;
        return nullptr;
    }

    return std::get_if<core::TaskStepAction>(task.actions.data() + index);
}

inline void
expectShellCommand(const core::Task& task, std::size_t index, const std::string& expectedCommand)
{
    const auto* shell = shellActionAt(task, index);
    ASSERT_NE(shell, nullptr);
    if (shell == nullptr)
    {
        return;
    }
    EXPECT_EQ(shell->command, expectedCommand);
}

inline void expectShellCommand(const std::optional<core::Task>& task,
                               std::size_t index,
                               const std::string& expectedCommand)
{
    ASSERT_TRUE(task.has_value());
    if (!task.has_value())
    {
        return;
    }
    expectShellCommand(*task, index, expectedCommand);
}

inline void expectStepInvocation(const core::Task& task,
                                 std::size_t index,
                                 const std::string& expectedName,
                                 bool expectsConfig)
{
    const auto* step = stepActionAt(task, index);
    ASSERT_NE(step, nullptr);
    if (step == nullptr)
    {
        return;
    }
    EXPECT_EQ(step->stepName, expectedName);
    EXPECT_EQ(step->config != nullptr, expectsConfig);
}

inline void expectStepInvocation(const std::optional<core::Task>& task,
                                 std::size_t index,
                                 const std::string& expectedName,
                                 bool expectsConfig)
{
    ASSERT_TRUE(task.has_value());
    if (!task.has_value())
    {
        return;
    }
    expectStepInvocation(*task, index, expectedName, expectsConfig);
}

inline const core::TaskInvocationAction* taskInvocationAt(const core::Task& task, std::size_t index)
{
    if (index >= task.actions.size())
    {
        ADD_FAILURE() << "task action index out of range: " << index;
        return nullptr;
    }

    return std::get_if<core::TaskInvocationAction>(task.actions.data() + index);
}

inline void expectTaskInvocation(const core::Task& task,
                                 std::size_t index,
                                 const std::string& expectedTaskName)
{
    const auto* invocation = taskInvocationAt(task, index);
    ASSERT_NE(invocation, nullptr);
    if (invocation == nullptr)
    {
        return;
    }
    EXPECT_EQ(invocation->taskName, expectedTaskName);
}

inline void expectTaskInvocation(const std::optional<core::Task>& task,
                                 std::size_t index,
                                 const std::string& expectedTaskName)
{
    ASSERT_TRUE(task.has_value());
    if (!task.has_value())
    {
        return;
    }
    expectTaskInvocation(*task, index, expectedTaskName);
}

inline void expectPhaseInvocation(const core::Task& task,
                                  std::size_t index,
                                  const std::string& expectedPhase,
                                  const std::string& expectedScope)
{
    if (index >= task.actions.size())
    {
        ADD_FAILURE() << "task action index out of range: " << index;
        return;
    }

    const auto* phaseAction = std::get_if<core::TaskPhaseAction>(task.actions.data() + index);
    ASSERT_NE(phaseAction, nullptr);
    if (phaseAction == nullptr)
    {
        return;
    }
    EXPECT_EQ(phaseAction->invocation.phase, expectedPhase);
    EXPECT_EQ(phaseAction->invocation.scope, expectedScope);
}

inline void expectMixedTaskWithStepInvocation(const core::Task& task)
{
    ASSERT_EQ(task.actions.size(), 3U);
    expectShellCommand(task, 0, "echo start");
    expectStepInvocation(task, 1, "cpp:compile", false);
    expectShellCommand(task, 2, "echo done");
}

inline void expectMixedTaskWithStepInvocation(const std::optional<core::Task>& task)
{
    ASSERT_TRUE(task.has_value());
    if (!task.has_value())
    {
        return;
    }
    expectMixedTaskWithStepInvocation(*task);
}

inline std::optional<core::Workflow> requireWorkflow(const core::Registry& registry,
                                                     const std::string& name)
{
    const auto Found = registry.findWorkflow(name);
    EXPECT_TRUE(Found.has_value());
    return Found;
}

inline void expectWorkflowStep(const core::WorkflowStep& step,
                               const std::string& phase,
                               const std::string& scope)
{
    EXPECT_EQ(step.invocation.phase, phase);
    EXPECT_EQ(step.invocation.scope, scope);
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
