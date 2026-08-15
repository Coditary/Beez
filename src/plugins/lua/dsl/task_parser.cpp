#include "beez/plugin/lua/dsl/task_parser.hpp"

#include "beez/core/model/phase_invocation.hpp"
#include "beez/core/model/phase_scope_reference.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/plugin/lua/dsl/task_step_reference.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

[[nodiscard]] std::vector<core::PhaseInvocation>
parseTaskPhaseInvocations(const sol::table& actionTable)
{
    const sol::object PhaseValue = actionTable["phase"];
    if (!isPresent(PhaseValue) || !PhaseValue.is<std::string>() ||
        PhaseValue.as<std::string>().empty())
    {
        throw std::runtime_error("task phase action is missing required field 'phase'");
    }

    const core::ScopedReference Scoped = core::parseScopedReference(PhaseValue.as<std::string>());
    if (Scoped.scopes.empty())
    {
        return {core::PhaseInvocation {.phase = Scoped.name, .scope = {}}};
    }

    std::vector<core::PhaseInvocation> invocations;
    invocations.reserve(Scoped.scopes.size());
    std::ranges::transform(
        Scoped.scopes,
        std::back_inserter(invocations),
        [&](const std::string& scope)
        { return core::PhaseInvocation {.phase = Scoped.name, .scope = scope}; });

    return invocations;
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::vector<core::TaskAction> parseTaskActions(const sol::table& actionsTable,
                                               const std::shared_ptr<sol::state>& luaState)
{
    std::vector<core::TaskAction> actions;
    actionsTable.for_each(
        [&actions, &luaState](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }

            if (value.is<std::string>())
            {
                actions.push_back(core::makeShellAction(value.as<std::string>()));
                return;
            }

            if (!value.is<sol::table>())
            {
                throw std::runtime_error(
                    "task action list entries must be strings or action tables");
            }

            const sol::table ActionTable = value.as<sol::table>();
            rejectDeprecatedTaskFields(ActionTable);

            const sol::object TaskValue = ActionTable["task"];
            const sol::object StepValue = ActionTable["step"];
            const sol::object PhaseValue = ActionTable["phase"];
            const bool HasTask = isPresent(TaskValue);
            const bool HasStep = isPresent(StepValue);
            const bool HasPhase = isPresent(PhaseValue);

            if (HasTask + HasStep + HasPhase > 1)
            {
                throw std::runtime_error(
                    "task action must set exactly one of 'task', 'step', or 'phase'");
            }

            if (HasTask)
            {
                if (!TaskValue.is<std::string>())
                {
                    throw std::runtime_error("task action field 'task' must be a string");
                }

                const std::string InvokedTask = TaskValue.as<std::string>();
                if (InvokedTask.empty())
                {
                    throw std::runtime_error("task action field 'task' must not be empty");
                }

                actions.push_back(core::makeTaskInvocation(InvokedTask));
                return;
            }

            if (HasPhase)
            {
                const auto Invocations = parseTaskPhaseInvocations(ActionTable);
                std::ranges::transform(Invocations,
                                       std::back_inserter(actions),
                                       [](const core::PhaseInvocation& invocation)
                                       { return core::makePhaseAction(invocation); });
                return;
            }

            const std::vector<std::string> StepReferences = parseTaskStepReferences(ActionTable);

            const sol::object ConfigValue = ActionTable["config"];
            core::StepConfigPtr sharedConfig;
            if (ConfigValue.valid())
            {
                if (!ConfigValue.is<sol::table>())
                {
                    throw std::runtime_error("task step action field 'config' must be a table");
                }

                sharedConfig = makeLuaStepConfig(luaState, ConfigValue.as<sol::table>());
            }

            for (const std::string& stepReference : StepReferences)
            {
                core::TaskStepAction stepAction;
                stepAction.stepName = stepReference;
                stepAction.config = sharedConfig;
                actions.emplace_back(std::move(stepAction));
            }
        });

    if (actions.empty())
    {
        throw std::runtime_error("task action list must not be empty");
    }

    return actions;
}

bool isTaskActionListTable(const sol::table& table)
{
    if (table.empty())
    {
        return false;
    }

    bool hasActionEntry = false;
    table.for_each(
        [&hasActionEntry](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }

            if (value.is<std::string>())
            {
                hasActionEntry = true;
                return;
            }

            if (value.is<sol::table>())
            {
                const sol::table ActionTable = value.as<sol::table>();
                const sol::object StepValue = ActionTable["step"];
                const sol::object TaskValue = ActionTable["task"];
                const sol::object PhaseValue = ActionTable["phase"];
                if ((StepValue.valid() && StepValue.is<std::string>()) ||
                    (TaskValue.valid() && TaskValue.is<std::string>()) ||
                    (PhaseValue.valid() && PhaseValue.is<std::string>()))
                {
                    hasActionEntry = true;
                }
            }
        });

    return hasActionEntry;
}

bool isPluginTaskImportTable(const sol::table& table)
{
    const sol::object PluginValue = table["plugin"];
    const sol::object TaskValue = table["task"];
    if (!isPresent(PluginValue) || !PluginValue.is<std::string>() ||
        PluginValue.as<std::string>().empty())
    {
        return false;
    }

    if (!isPresent(TaskValue) || !TaskValue.is<std::string>() ||
        TaskValue.as<std::string>().empty())
    {
        return false;
    }

    bool hasNumericActionEntry = false;
    table.for_each(
        [&hasNumericActionEntry](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }

            if (value.is<std::string>() || value.is<sol::table>())
            {
                hasNumericActionEntry = true;
            }
        });

    return !hasNumericActionEntry;
}

std::string buildPluginTaskReference(const sol::table& importTable)
{
    if (!isPluginTaskImportTable(importTable))
    {
        throw std::runtime_error(
            "task import table must contain non-empty string fields 'plugin' and 'task'");
    }

    const std::string Plugin = importTable.get<std::string>("plugin");
    const std::string TaskName = importTable.get<std::string>("task");
    if (Plugin.find('/') == std::string::npos)
    {
        throw std::runtime_error(
            "task import field 'plugin' must use the form 'organization/plugin'");
    }

    return Plugin + ':' + TaskName;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
