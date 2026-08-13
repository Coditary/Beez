#include "beez/plugin/lua/dsl/task_parser.hpp"

#include "beez/core/model/task_action.hpp"
#include "beez/plugin/lua/dsl/task_step_reference.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"

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

}  // namespace

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
            const bool HasTask = isPresent(TaskValue);
            const bool HasStep = isPresent(StepValue);

            if (HasTask && HasStep)
            {
                throw std::runtime_error("task action cannot set both 'task' and 'step'");
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

            core::TaskStepAction stepAction;
            stepAction.stepName = parseTaskStepReference(ActionTable);

            const sol::object ConfigValue = ActionTable["config"];
            if (ConfigValue.valid())
            {
                if (!ConfigValue.is<sol::table>())
                {
                    throw std::runtime_error("task step action field 'config' must be a table");
                }

                stepAction.config = makeLuaStepConfig(luaState, ConfigValue.as<sol::table>());
            }

            actions.emplace_back(std::move(stepAction));
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
                if ((StepValue.valid() && StepValue.is<std::string>()) ||
                    (TaskValue.valid() && TaskValue.is<std::string>()))
                {
                    hasActionEntry = true;
                }
            }
        });

    return hasActionEntry;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
