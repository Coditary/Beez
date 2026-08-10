#include "beez/plugin/lua/settings/lua_settings.hpp"

#include "beez/core/config/paths/config_paths.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/plugin/lua/settings/settings_overlay.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

bool loadSettingsFromLuaFile(const std::filesystem::path& path, core::BeezSettings& settings)
{
    if (path.empty() || !std::filesystem::exists(path))
    {
        return true;
    }

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package);

    const sol::protected_function_result Result =
        lua.safe_script_file(path.string(), sol::script_pass_on_error);
    if (!Result.valid())
    {
        const sol::error Error = Result;
        throw std::runtime_error(std::string("failed to load settings from ") + path.string() +
                                 ": " + Error.what());
    }

    const sol::object Returned = Result.get<sol::object>();
    if (!Returned.valid() || Returned.get_type() == sol::type::lua_nil)
    {
        return true;
    }

    if (!Returned.is<sol::table>())
    {
        throw std::runtime_error("settings file must return a table");
    }

    mergeSettingsFromLuaTable(Returned.as<sol::table>(), settings);
    return true;
}

void tryLoadGlobalBeezSettings(core::BeezSettings& settings)
{
    const auto ConfigPath = core::globalBeezConfigPath();
    if (ConfigPath.empty())
    {
        return;
    }

    static_cast<void>(loadSettingsFromLuaFile(ConfigPath, settings));
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
