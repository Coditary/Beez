#include "beez/plugin/lua/dsl/workflow_parser.hpp"

#include "beez/core/phase_invocation.hpp"
#include "beez/core/workflow_step.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

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

}  // namespace

core::Workflow parseWorkflow(const std::string& name, const sol::table& stepsTable)
{
    core::Workflow workflow;
    workflow.name = name;

    stepsTable.for_each(
        [&workflow, &name](const sol::object& /*key*/, const sol::object& value)
        {
            if (!value.is<sol::table>())
            {
                if (value.is<std::string>())
                {
                    throw std::runtime_error("workflow '" + name +
                                             "' entry must be a phase table, not a string ('" +
                                             value.as<std::string>() + "')");
                }

                if (value.is<int>() || value.is<double>())
                {
                    throw std::runtime_error("workflow '" + name +
                                             "' entry must be a phase table, not a number");
                }

                return;
            }

            workflow.steps.push_back(parseWorkflowStep(value.as<sol::table>()));
        });

    return workflow;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
