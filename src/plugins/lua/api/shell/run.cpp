#include "beez/plugin/lua/api/shell/run.hpp"

#include <iostream>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] bool optionEnabled(const sol::optional<sol::table>& options, const char* key)
{
    if (!options.has_value())
    {
        return false;
    }

    const sol::optional<bool> Value = (*options)[key];
    return Value.value_or(false);
}

}  // namespace

sol::object runShellCommand(sol::object context,
                            const std::string& logPrefix,
                            const std::string& command,
                            const sol::optional<sol::table>& options)
{
    sol::table ctx = context.as<sol::table>();
    sol::state_view lua = ctx.lua_state();

    sol::table spawnOptions = lua.create_table();
    spawnOptions["cmd"] = command;
    const sol::object handle = ctx["spawn"](ctx, spawnOptions);

    sol::table waitOptions = lua.create_table();
    waitOptions["exitCode"] = true;
    waitOptions["output"] = true;
    const sol::object waitResult = ctx["wait"](ctx, handle, waitOptions);

    const sol::table result = waitResult.as<sol::table>();
    const int ExitCode = result.get_or("exitCode", 0);
    const std::string Output = result.get_or<std::string>("output", "");
    const bool ReturnOutput = optionEnabled(options, "return_output");

    if (ExitCode != 0)
    {
        std::cout << logPrefix << " failed (exit " << ExitCode << ")\n";
        if (!Output.empty())
        {
            std::cout << Output << '\n';
        }

        if (ReturnOutput)
        {
            sol::stack::push(lua, ExitCode);
            sol::stack::push(lua, Output);
            return sol::object(lua, sol::in_place, 2);
        }

        return sol::make_object(lua, ExitCode);
    }

    const sol::optional<bool> Verbose = ctx["verbose"];
    if (Verbose.value_or(false) && !Output.empty())
    {
        std::cout << Output << '\n';
    }

    if (ReturnOutput)
    {
        sol::stack::push(lua, 0);
        sol::stack::push(lua, Output);
        return sol::object(lua, sol::in_place, 2);
    }

    return sol::make_object(lua, 0);
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
