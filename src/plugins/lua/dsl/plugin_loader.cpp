#include "beez/plugin/lua/dsl/plugin_loader.hpp"

#include "beez/core/plugin/paths.hpp"
#include "beez/plugin/lua/api/beez_table.hpp"
#include "beez/plugin/lua/dsl/step_parser.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

void registerPluginSteps(const sol::table& stepsTable,
                         core::Registry& registry,
                         const std::shared_ptr<sol::state>& luaState)
{
    for (const auto& [key, value] : stepsTable)
    {
        if (!key.is<std::string>())
        {
            throw std::runtime_error("plugin steps must use string keys as step names");
        }

        const std::string StepName = key.as<std::string>();
        sol::object StepValue = value;

        if (StepValue.is<sol::protected_function>())
        {
            const sol::protected_function LazyLoader = StepValue.as<sol::protected_function>();
            const sol::protected_function_result Result = LazyLoader();
            if (!Result.valid())
            {
                const sol::error LuaError = Result;
                throw std::runtime_error("plugin step '" + StepName +
                                         "' lazy loader failed: " + LuaError.what());
            }

            if (Result.return_count() == 0)
            {
                throw std::runtime_error("plugin step '" + StepName +
                                         "' lazy loader must return a step table");
            }

            StepValue = Result.get<sol::object>(0);
        }

        if (!StepValue.is<sol::table>())
        {
            throw std::runtime_error("plugin step '" + StepName + "' must be a table or function");
        }

        sol::table StepTable = StepValue.as<sol::table>();
        if (!StepTable["name"].valid())
        {
            StepTable["name"] = StepName;
        }

        registry.registerStep(parseStepTable(StepTable, luaState));
    }
}

void loadPluginScript(const std::filesystem::path& scriptPath,
                      const BeezPluginRef& pluginRef,
                      core::Registry& registry,
                      const core::Context& context)
{
    const auto PluginState = std::make_shared<sol::state>();
    PluginState->open_libraries(sol::lib::base,
                                sol::lib::package,
                                sol::lib::string,
                                sol::lib::table,
                                sol::lib::math);

    core::BeezSettings pluginSettings;
    registerBeezApi(PluginState, context, pluginSettings);

    const auto PluginDirectory = scriptPath.parent_path();
    const std::string PluginPathPrefix = PluginDirectory.string() + "/?.lua;" +
                                         PluginDirectory.string() + "/?/init.lua";
    sol::table PackageTable = PluginState->globals()["package"];
    const std::string ExistingPath = PackageTable["path"];
    PackageTable["path"] = PluginPathPrefix + ";" + ExistingPath;

    (*PluginState)["plugin"] = [&registry, &pluginRef, PluginState](const std::string& name,
                                                                    const sol::table& options)
    {
        if (name != pluginRef.name)
        {
            throw std::runtime_error("plugin name '" + name + "' does not match required name '" +
                                     pluginRef.name + "'");
        }

        const sol::object VersionObject = options["version"];
        if (!VersionObject.is<std::string>())
        {
            throw std::runtime_error("plugin '" + name + "' requires string version");
        }

        if (VersionObject.as<std::string>() != pluginRef.version)
        {
            throw std::runtime_error("plugin '" + name + "' version '" +
                                     VersionObject.as<std::string>() +
                                     "' does not match required version '" + pluginRef.version +
                                     "'");
        }

        const sol::object StepsObject = options["steps"];
        if (!StepsObject.valid() || !StepsObject.is<sol::table>())
        {
            throw std::runtime_error("plugin '" + name + "' requires a steps table");
        }

        registerPluginSteps(StepsObject.as<sol::table>(), registry, PluginState);
    };

    PluginState->script_file(scriptPath.string());
}

}  // namespace

void loadBeezPlugins(const std::vector<BeezPluginRef>& plugins,
                     core::Registry& registry,
                     const core::Context& context)
{
    for (const auto& pluginRef : plugins)
    {
        const auto ScriptPath = core::findPluginScript(pluginRef.name, pluginRef.version);
        if (!ScriptPath.has_value())
        {
            throw std::runtime_error("beez plugin '" + pluginRef.name + "@" + pluginRef.version +
                                     "' was not found in the plugin cache");
        }

        loadPluginScript(*ScriptPath, pluginRef, registry, context);
    }
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
