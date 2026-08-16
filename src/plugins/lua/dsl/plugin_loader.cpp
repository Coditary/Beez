#include "beez/plugin/lua/dsl/plugin_loader.hpp"

#include "beez/core/plugin/installer.hpp"
#include "beez/plugin/lua/api/beez_table.hpp"
#include "beez/plugin/lua/dsl/step_parser.hpp"
#include "beez/plugin/lua/dsl/task_parser.hpp"
#include "beez/plugin/lua/dsl/workflow_parser.hpp"
#include "beez/plugin/lua/runtime/plugin_config.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

// NOLINTBEGIN(readability-identifier-naming)

namespace
{

[[nodiscard]] std::pair<std::string, std::string> splitPluginName(const std::string& qualifiedName)
{
    const auto SlashPosition = qualifiedName.find('/');
    if (SlashPosition == std::string::npos)
    {
        throw std::runtime_error("reqpack.beez plugin name '" + qualifiedName +
                                 "' must use the form 'organization/plugin'");
    }

    if (SlashPosition == 0 || SlashPosition == qualifiedName.size() - 1)
    {
        throw std::runtime_error("reqpack.beez plugin name '" + qualifiedName +
                                 "' must use the form 'organization/plugin'");
    }

    return {qualifiedName.substr(0, SlashPosition), qualifiedName.substr(SlashPosition + 1)};
}

[[nodiscard]] BeezPluginRef parsePluginRefEntry(const sol::object& entry)
{
    if (!entry.is<sol::table>())
    {
        throw std::runtime_error(
            "reqpack.beez entries must be tables with name and either path or version");
    }

    const sol::table PluginTable = entry.as<sol::table>();
    const sol::object NameObject = PluginTable["name"];
    if (!NameObject.is<std::string>())
    {
        throw std::runtime_error("reqpack.beez plugin entry requires string name");
    }

    BeezPluginRef pluginRef;
    const auto [Organization, PluginName] = splitPluginName(NameObject.as<std::string>());
    pluginRef.organization = Organization;
    pluginRef.name = PluginName;

    const sol::object PathObject = PluginTable["path"];
    if (PathObject.valid() && PathObject.get_type() != sol::type::lua_nil)
    {
        if (!PathObject.is<std::string>())
        {
            throw std::runtime_error("reqpack.beez plugin path must be a string");
        }
        pluginRef.path = PathObject.as<std::string>();
    }

    const sol::object VersionObject = PluginTable["version"];
    if (VersionObject.valid() && VersionObject.get_type() != sol::type::lua_nil)
    {
        if (!VersionObject.is<std::string>())
        {
            throw std::runtime_error("reqpack.beez plugin version must be a string");
        }
        pluginRef.version = VersionObject.as<std::string>();
    }

    const sol::object SourceObject = PluginTable["source"];
    if (SourceObject.valid() && SourceObject.get_type() != sol::type::lua_nil)
    {
        if (!SourceObject.is<std::string>())
        {
            throw std::runtime_error("reqpack.beez plugin source must be a string");
        }
        pluginRef.source = SourceObject.as<std::string>();
    }

    if (!pluginRef.isLocal() && !pluginRef.version.has_value())
    {
        throw std::runtime_error("remote reqpack.beez plugin '" + pluginRef.organization + '/' +
                                 pluginRef.name + "' requires a version pin");
    }

    return pluginRef;
}

void registerPluginSteps(const sol::table& stepsTable,
                         core::Registry& registry,
                         const BeezPluginRef& pluginRef,
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

        core::Step step = parseStepTable(StepTable, luaState);
        const sol::object ConfigValue = StepTable["config"];
        if (ConfigValue.valid() && ConfigValue.is<sol::table>())
        {
            const std::string PluginKey = pluginRef.organization + "/" + pluginRef.name;
            step.config = makePluginStepConfig(luaState, PluginKey, ConfigValue.as<sol::table>());
        }

        const std::optional<std::string> StepVersion = pluginRef.version;
        const bool AllowUnversionedAliases = !pluginRef.fromInstalledCache;
        registry.registerPluginStep(
            step, pluginRef.organization, pluginRef.name, StepVersion, AllowUnversionedAliases);
    }
}

void registerPluginWorkflows(const sol::table& workflowsTable,
                             core::Registry& registry,
                             const BeezPluginRef& pluginRef)
{
    for (const auto& [key, value] : workflowsTable)
    {
        if (!key.is<std::string>())
        {
            throw std::runtime_error("plugin workflows must use string keys as workflow names");
        }

        if (!value.is<sol::table>())
        {
            throw std::runtime_error("plugin workflow '" + key.as<std::string>() +
                                     "' must be a table");
        }

        core::Workflow workflow = parseWorkflow(key.as<std::string>(), value.as<sol::table>());
        registry.registerPluginWorkflow(
            std::move(workflow), pluginRef.organization, pluginRef.name);
    }
}

void registerPluginTasks(const sol::table& tasksTable,
                         core::Registry& registry,
                         const BeezPluginRef& pluginRef,
                         const std::shared_ptr<sol::state>& luaState)
{
    for (const auto& [key, value] : tasksTable)
    {
        if (!key.is<std::string>())
        {
            throw std::runtime_error("plugin tasks must use string keys as task names");
        }

        if (!value.is<sol::table>())
        {
            throw std::runtime_error("plugin task '" + key.as<std::string>() + "' must be a table");
        }

        core::Task task;
        task.name = key.as<std::string>();
        task.actions = parseTaskActions(value.as<sol::table>(), luaState);
        registry.registerPluginTask(std::move(task), pluginRef.organization, pluginRef.name);
    }
}

void loadPluginScript(const std::filesystem::path& scriptPath,
                      const BeezPluginRef& pluginRef,
                      core::Registry& registry,
                      const core::Context& context)
{
    const auto PluginState = std::make_shared<sol::state>();
    PluginState->open_libraries(
        sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math);

    core::BeezSettings pluginSettings;
    registerBeezApi(PluginState, context, pluginSettings);

    const auto PluginDirectory = scriptPath.parent_path();
    const std::string PluginPathPrefix =
        PluginDirectory.string() + "/?.lua;" + PluginDirectory.string() + "/?/init.lua";
    sol::table PackageTable = PluginState->globals()["package"];
    const std::string ExistingPath = PackageTable["path"];
    PackageTable["path"] = PluginPathPrefix + ";" + ExistingPath;

    (*PluginState)["plugin"] =
        [&registry, &pluginRef, PluginState](const std::string& name, const sol::table& options)
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

        if (pluginRef.version.has_value() &&
            VersionObject.as<std::string>() != pluginRef.version.value())
        {
            throw std::runtime_error(
                "plugin '" + name + "' version '" + VersionObject.as<std::string>() +
                "' does not match required version '" + pluginRef.version.value() + "'");
        }

        const sol::object ConfigObject = options["config"];
        if (ConfigObject.valid())
        {
            if (!ConfigObject.is<sol::table>())
            {
                throw std::runtime_error("plugin '" + name + "' field 'config' must be a table");
            }

            const std::string PluginKey = pluginRef.organization + "/" + pluginRef.name;
            registerPluginConfigDefinition(PluginKey, PluginState, ConfigObject.as<sol::table>());
        }

        const sol::object StepsObject = options["steps"];
        if (!StepsObject.valid() || !StepsObject.is<sol::table>())
        {
            throw std::runtime_error("plugin '" + name + "' requires a steps table");
        }

        registerPluginSteps(StepsObject.as<sol::table>(), registry, pluginRef, PluginState);
    };

    (*PluginState)["workflows"] = [&registry, &pluginRef](const sol::table& workflowsTable)
    { registerPluginWorkflows(workflowsTable, registry, pluginRef); };

    (*PluginState)["tasks"] = [&registry, &pluginRef, PluginState](const sol::table& tasksTable)
    { registerPluginTasks(tasksTable, registry, pluginRef, PluginState); };

    PluginState->script_file(scriptPath.string());
}

[[nodiscard]] std::filesystem::path resolvePluginScript(const BeezPluginRef& pluginRef,
                                                        const core::Context& context)
{
    if (!pluginRef.path.has_value())
    {
        throw std::runtime_error("beez plugin '" + pluginRef.organization + '/' + pluginRef.name +
                                 "' does not have a local path");
    }

    std::filesystem::path PluginDirectory = context.projectRoot() / *pluginRef.path;
    if (pluginRef.version.has_value())
    {
        PluginDirectory /= pluginRef.version.value();
    }

    const auto ScriptPath = PluginDirectory / "beez_plugin.lua";
    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(ScriptPath, errorCode) || errorCode)
    {
        throw std::runtime_error("beez plugin '" + pluginRef.organization + "/" + pluginRef.name +
                                 "' was not found at path '" + ScriptPath.string() + "'");
    }

    return ScriptPath;
}

}  // namespace

std::vector<BeezPluginRef> parseBeezPluginTable(const sol::table& table)
{
    std::vector<BeezPluginRef> plugins;
    for (const auto& [key, value] : table)
    {
        if (!key.is<int>())
        {
            throw std::runtime_error("reqpack.beez must be an array of plugin references");
        }
        plugins.push_back(parsePluginRefEntry(value));
    }
    return plugins;
}

void loadBeezPlugins(const std::vector<BeezPluginRef>& plugins,
                     core::Registry& registry,
                     const core::Context& context)
{
    for (const auto& pluginRef : plugins)
    {
        if (!pluginRef.isLocal())
        {
            continue;
        }

        const auto ScriptPath = resolvePluginScript(pluginRef, context);
        loadPluginScript(ScriptPath, pluginRef, registry, context);
    }
}

void loadInstalledBeezPlugin(const std::string& organization,
                             const std::string& name,
                             const std::string& version,
                             core::Registry& registry,
                             const core::Context& context)
{
    if (!tryLoadInstalledBeezPlugin(organization, name, version, registry, context))
    {
        throw std::runtime_error("installed beez plugin '" + organization + '/' + name + '@' +
                                 version + "' was not found");
    }
}

bool tryLoadInstalledBeezPlugin(const std::string& organization,
                                const std::string& name,
                                const std::string& version,
                                core::Registry& registry,
                                const core::Context& context)
{
    if (registry.hasPluginVersionLoaded(organization, name, version))
    {
        return true;
    }

    const auto ScriptResult = core::resolveInstalledBeezPluginScript(organization, name, version);
    if (!ScriptResult.hasValue())
    {
        return false;
    }

    BeezPluginRef pluginRef;
    pluginRef.organization = organization;
    pluginRef.name = name;
    pluginRef.version = version;
    pluginRef.fromInstalledCache = true;
    loadPluginScript(ScriptResult.value(), pluginRef, registry, context);
    return true;
}

}  // namespace beez::plugin::lua
// NOLINTEND(readability-identifier-naming)
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
