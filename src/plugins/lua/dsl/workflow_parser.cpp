#include "beez/plugin/lua/dsl/workflow_parser.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_scope_reference.hpp"
#include "beez/core/model/workflow_stage.hpp"
#include "beez/core/model/workflow_step.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] bool isPresent(const sol::object& value)
{
    return value.valid() && value.get_type() != sol::type::lua_nil;
}

core::PhaseInvocation parsePhaseInvocation(const sol::table& invocationTable)
{
    const sol::object PhaseValue = invocationTable["phase"];
    if (!PhaseValue.valid() || !PhaseValue.is<std::string>() ||
        PhaseValue.as<std::string>().empty())
    {
        throw std::runtime_error("workflow phase invocation is missing required field 'phase'");
    }

    const sol::object ScopeValue = invocationTable["scope"];
    if (!ScopeValue.valid() || !ScopeValue.is<std::string>() ||
        ScopeValue.as<std::string>().empty())
    {
        throw std::runtime_error("workflow phase invocation is missing required field 'scope'");
    }

    return core::PhaseInvocation {.phase = PhaseValue.as<std::string>(),
                                  .scope = ScopeValue.as<std::string>()};
}

core::WorkflowStep parseWorkflowStep(const sol::table& stepTable)
{
    if (stepTable["parallel"].valid())
    {
        throw std::runtime_error("workflow step does not support 'parallel'");
    }

    return core::WorkflowStep {.invocation = parsePhaseInvocation(stepTable)};
}

[[nodiscard]] bool isStagedWorkflowEntry(const sol::table& entryTable)
{
    if (entryTable["phase"].valid())
    {
        return false;
    }

    const sol::object StageNameValue = entryTable[1];
    const sol::object InvocationsValue = entryTable[2];
    return isPresent(StageNameValue) && StageNameValue.is<std::string>() &&
           !StageNameValue.as<std::string>().empty() && isPresent(InvocationsValue) &&
           InvocationsValue.is<sol::table>();
}

core::WorkflowStage parseWorkflowStage(const std::string& workflowName,
                                       const sol::table& stageTable)
{
    const sol::object StageNameValue = stageTable[1];
    const sol::object InvocationsValue = stageTable[2];
    const std::string StageName = StageNameValue.as<std::string>();
    const sol::table InvocationsTable = InvocationsValue.as<sol::table>();

    core::WorkflowStage stage;
    stage.name = StageName;

    InvocationsTable.for_each(
        [&workflowName, &stageName = stage.name, &stage](const sol::object& key,
                                                         const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }

            if (!value.is<std::string>())
            {
                throw std::runtime_error(
                    "workflow '" + workflowName + "' stage '" + stageName +
                    "' entries must be phase[scope] or unscoped phase strings");
            }

            stage.invocations.push_back(core::parseWorkflowPhaseReference(value.as<std::string>()));
        });

    return stage;
}

void ensureWorkflowFormat(const std::string& workflowName,
                          std::optional<bool>& stagedFormat,
                          const bool entryIsStaged)
{
    if (!stagedFormat.has_value())
    {
        stagedFormat = entryIsStaged;
        return;
    }

    if (*stagedFormat != entryIsStaged)
    {
        throw std::runtime_error("workflow '" + workflowName +
                                 "' cannot mix staged and legacy entries");
    }
}

void registerWorkflowStage(const std::string& workflowName,
                           core::Workflow& workflow,
                           std::unordered_set<std::string>& stageNames,
                           core::WorkflowStage stage)
{
    if (!stageNames.insert(stage.name).second)
    {
        throw std::runtime_error("workflow '" + workflowName + "' defines duplicate stage '" +
                                 stage.name + "'");
    }

    workflow.stages.push_back(std::move(stage));
}

}  // namespace

core::Workflow parseWorkflow(const std::string& name, const sol::table& stepsTable)
{
    core::Workflow workflow;
    workflow.name = name;

    std::optional<bool> stagedFormat;
    std::unordered_set<std::string> stageNames;

    stepsTable.for_each(
        [&workflow, &name, &stagedFormat, &stageNames](const sol::object& /*key*/,
                                                       const sol::object& value)
        {
            if (!value.is<sol::table>())
            {
                if (value.is<std::string>())
                {
                    ensureWorkflowFormat(name, stagedFormat, false);
                    workflow.steps.push_back(core::WorkflowStep {
                        .invocation = core::parseWorkflowPhaseReference(value.as<std::string>())});
                    return;
                }

                if (value.is<int>() || value.is<double>())
                {
                    throw std::runtime_error("workflow '" + name +
                                             "' entry must be a phase table, not a number");
                }

                return;
            }

            const sol::table EntryTable = value.as<sol::table>();
            if (isStagedWorkflowEntry(EntryTable))
            {
                ensureWorkflowFormat(name, stagedFormat, true);
                registerWorkflowStage(
                    name, workflow, stageNames, parseWorkflowStage(name, EntryTable));
                return;
            }

            ensureWorkflowFormat(name, stagedFormat, false);
            workflow.steps.push_back(parseWorkflowStep(EntryTable));
        });

    return workflow;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
