#include "beez/plugin/lua/dsl/step_parser.hpp"

#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

std::vector<std::string> parseStringArrayField(const sol::table& options,
                                               const std::string& fieldName,
                                               const std::string& stepName)
{
    const sol::object FieldValue = options[fieldName];
    if (!FieldValue.valid())
    {
        return {};
    }

    if (!FieldValue.is<sol::table>())
    {
        throw std::runtime_error("step '" + stepName + "' field '" + fieldName +
                                 "' must be a table of strings");
    }

    std::vector<std::string> values;
    const sol::table FieldTable = FieldValue.as<sol::table>();
    FieldTable.for_each(
        [&values, &fieldName, &stepName](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }

            if (!value.is<std::string>())
            {
                throw std::runtime_error("step '" + stepName + "' field '" + fieldName +
                                         "' must contain only strings");
            }

            values.push_back(value.as<std::string>());
        });

    return values;
}

core::Step parseStepTable(const sol::table& options, const std::shared_ptr<sol::state>& luaState)
{
    core::Step step;

    const sol::object NameValue = options["name"];
    if (!NameValue.valid() || !NameValue.is<std::string>())
    {
        throw std::runtime_error("step is missing required field 'name'");
    }
    step.name = NameValue.as<std::string>();

    const sol::object PhaseValue = options["phase"];
    if (!PhaseValue.valid() || !PhaseValue.is<std::string>())
    {
        throw std::runtime_error("step '" + step.name + "' is missing required field 'phase'");
    }
    step.phase = PhaseValue.as<std::string>();

    const sol::object ScopeValue = options["scope"];
    if (!ScopeValue.valid() || !ScopeValue.is<std::string>())
    {
        throw std::runtime_error("step '" + step.name + "' is missing required field 'scope'");
    }
    step.scope = ScopeValue.as<std::string>();

    const sol::object DescriptionValue = options["description"];
    if (DescriptionValue.valid())
    {
        if (!DescriptionValue.is<std::string>())
        {
            throw std::runtime_error("step '" + step.name +
                                     "' field 'description' must be a string");
        }
        step.description = DescriptionValue.as<std::string>();
    }

    const sol::object ConfigValue = options["config"];
    if (ConfigValue.valid())
    {
        if (!ConfigValue.is<sol::table>())
        {
            throw std::runtime_error("step '" + step.name + "' field 'config' must be a table");
        }

        step.config = makeLuaStepConfig(luaState, ConfigValue.as<sol::table>());
    }

    step.input = parseStringArrayField(options, "input", step.name);
    step.output = parseStringArrayField(options, "output", step.name);
    step.mutate = parseStringArrayField(options, "mutate", step.name);

    const sol::object RunValue = options["run"];
    if (!RunValue.valid())
    {
        throw std::runtime_error("step '" + step.name + "' is missing required field 'run'");
    }

    if (RunValue.is<std::string>())
    {
        step.shellRun = RunValue.as<std::string>();
        return step;
    }

    if (RunValue.is<sol::protected_function>())
    {
        const sol::protected_function LuaFunction = RunValue.as<sol::protected_function>();
        step.callback = [luaState, LuaFunction](const core::Context& context) mutable -> int
        {
            const sol::table StepContext = bindStepContext(luaState, context);
            const sol::protected_function_result Result = LuaFunction(StepContext);
            if (!Result.valid())
            {
                const sol::error LuaError = Result;
                std::cerr << "Lua step error: " << LuaError.what() << '\n';
                return 1;
            }

            if (Result.return_count() == 0)
            {
                return 0;
            }

            const sol::object ReturnValue = Result.get<sol::object>(0);
            if (ReturnValue.is<int>())
            {
                return ReturnValue.as<int>();
            }

            return 0;
        };
        return step;
    }

    throw std::runtime_error("step '" + step.name + "' field 'run' must be a string or function");
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
