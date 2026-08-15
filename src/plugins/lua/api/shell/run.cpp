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

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
sol::object runShellCommand(const sol::object& context,
                            const std::string& logPrefix,
                            const std::string& command,
                            const sol::optional<sol::table>& options)
{
    sol::table ctx = context.as<sol::table>();
    sol::state_view lua = ctx.lua_state();

    sol::table spawnOptions = lua.create_table();
    spawnOptions["cmd"] = command;
    const sol::object Handle = ctx["spawn"](ctx, spawnOptions);

    sol::table waitOptions = lua.create_table();
    waitOptions["exitCode"] = true;
    waitOptions["output"] = true;
    const sol::object WaitResult = ctx["wait"](ctx, Handle, waitOptions);

    const sol::table Result = WaitResult.as<sol::table>();
    const int ExitCode = Result.get_or("exitCode", 0);
    const std::string Output = Result.get_or<std::string>("output", "");
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
            return sol::make_object(lua, std::make_pair(ExitCode, Output));
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
        return sol::make_object(lua, std::make_pair(0, Output));
    }

    return sol::make_object(lua, 0);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
