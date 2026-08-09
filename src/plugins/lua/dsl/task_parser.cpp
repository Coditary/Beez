#include "beez/plugin/lua/dsl/task_parser.hpp"

#include "beez/core/model/task_action.hpp"
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
                    "task action list entries must be strings or step invocation tables");
            }

            const sol::table StepTable = value.as<sol::table>();
            const sol::object NameValue = StepTable["name"];
            if (!NameValue.valid() || !NameValue.is<std::string>())
            {
                throw std::runtime_error("task step invocation is missing required field 'name'");
            }

            core::TaskStepAction stepAction;
            stepAction.stepName = NameValue.as<std::string>();

            const sol::object ConfigValue = StepTable["config"];
            if (ConfigValue.valid())
            {
                if (!ConfigValue.is<sol::table>())
                {
                    throw std::runtime_error("task step invocation field 'config' must be a table");
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
                const sol::table StepTable = value.as<sol::table>();
                const sol::object NameValue = StepTable["name"];
                if (NameValue.valid() && NameValue.is<std::string>())
                {
                    hasActionEntry = true;
                }
            }
        });

    return hasActionEntry;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
