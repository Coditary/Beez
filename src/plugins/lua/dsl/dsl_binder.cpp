#include "beez/plugin/lua/dsl/dsl_binder.hpp"

#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/plugin/lua/api/beez_table.hpp"
#include "beez/plugin/lua/dsl/configure_parser.hpp"
#include "beez/plugin/lua/dsl/plugin_loader.hpp"
#include "beez/plugin/lua/dsl/reqpack_parser.hpp"
#include "beez/plugin/lua/dsl/step_parser.hpp"
#include "beez/plugin/lua/dsl/task_parser.hpp"
#include "beez/plugin/lua/dsl/workflow_parser.hpp"
#include "beez/plugin/lua/dsl/workflows_parser.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"

#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include "beez/core/registry/task_reference.hpp"
#include "beez/core/registry/workflow_reference.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

// NOLINTBEGIN(readability-identifier-naming)

namespace
{

[[nodiscard]] std::pair<std::string, std::string> splitQualifiedPluginName(const std::string& name)
{
    const auto SlashPosition = name.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0 ||
        SlashPosition == name.size() - 1)
    {
        throw std::runtime_error("configure_plugin plugin name '" + name +
                                 "' must use the form 'organization/plugin'");
    }

    return {name.substr(0, SlashPosition), name.substr(SlashPosition + 1)};
}

class DslBinder
{
  public:
    DslBinder(core::Registry* registry,
              std::weak_ptr<sol::state> luaState,
              ReqpackBeezPluginCatalog* reqpackBeezPlugins)
        : registry_(registry), luaState_(std::move(luaState)),
          reqpackBeezPlugins_(reqpackBeezPlugins)
    {
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- DSL binding mirrors Lua call order
    void task(const std::string& name, const std::string& run) const
    {
        core::Task task;
        task.name = name;
        task.actions = {core::makeShellAction(run)};
        registry_->registerTask(std::move(task));
    }

    void task(const std::string& name, const sol::table& actions) const
    {
        if (isPluginTaskImportTable(actions))
        {
            registerTaskFromPluginReference(name, buildPluginTaskReference(actions));
            return;
        }

        if (!isTaskActionListTable(actions))
        {
            throw std::runtime_error("task '" + name + "' table form must be a list of actions");
        }

        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        core::Task task;
        task.name = name;
        task.actions = parseTaskActions(actions, LuaState);
        registry_->registerTask(std::move(task));
    }

    void task(const std::string& pluginTaskReference) const
    {
        if (!core::looksLikePluginTaskReference(pluginTaskReference))
        {
            throw std::runtime_error("task reference '" + pluginTaskReference +
                                     "' must use the form 'organization/plugin:task'");
        }

        const core::PluginTaskRef ParsedReference =
            core::parsePluginTaskReference(pluginTaskReference);
        validatePluginTaskReference(pluginTaskReference);
        registry_->registerTaskFromPluginReference(ParsedReference.taskName, pluginTaskReference);
    }

    void registerTaskFromPluginReference(const std::string& localName,
                                         const std::string& pluginTaskReference) const
    {
        validatePluginTaskReference(pluginTaskReference);
        registry_->registerTaskFromPluginReference(localName, pluginTaskReference);
    }

    void validatePluginTaskReference(const std::string& pluginTaskReference) const
    {
        if (reqpackBeezPlugins_ == nullptr || reqpackBeezPlugins_->empty())
        {
            return;
        }

        const core::PluginTaskRef ParsedReference =
            core::parsePluginTaskReference(pluginTaskReference);
        if (!reqpackBeezPlugins_->find(ParsedReference.organization, ParsedReference.plugin)
                 .has_value())
        {
            throw std::runtime_error("task references plugin '" + ParsedReference.organization +
                                     '/' + ParsedReference.plugin +
                                     "' which is not declared in reqpack.beez");
        }
    }

    void step(const sol::table& options) const
    {
        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        registry_->registerStep(parseStepTable(options, LuaState));
    }

    void configureStep(const std::string& name,
                       const sol::table& configTable,
                       const std::optional<std::string>& profile = std::nullopt) const
    {
        if (profile.has_value())
        {
            if (*profile == "NONE")
            {
                if (registry_->profile().has_value())
                {
                    return;
                }
            }
            else
            {
                const auto& currentProfile = registry_->profile();
                if (!currentProfile.has_value() || *currentProfile != *profile)
                {
                    return;
                }
            }
        }

        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        registry_->configureStep(name, makeLuaStepConfig(LuaState, configTable));
    }

    void configure(const sol::table& entriesTable) const
    {
        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        parseConfigureTable(
            entriesTable, LuaState,
            [this](const std::string& qualifiedName, const sol::table& configTable,
                   const std::optional<std::string>& profile)
            { configurePlugin(qualifiedName, configTable, profile); },
            [this](const std::string& stepName, const sol::table& configTable,
                   const std::optional<std::string>& profile)
            { configureStep(stepName, configTable, profile); });
    }

    void configurePlugin(const std::string& qualifiedName,
                         const sol::table& configTable,
                         const std::optional<std::string>& profile = std::nullopt) const
    {
        if (profile.has_value())
        {
            if (*profile == "NONE")
            {
                if (registry_->profile().has_value())
                {
                    return;
                }
            }
            else
            {
                const auto& currentProfile = registry_->profile();
                if (!currentProfile.has_value() || *currentProfile != *profile)
                {
                    return;
                }
            }
        }

        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        const auto [Organization, Plugin] = splitQualifiedPluginName(qualifiedName);
        if (reqpackBeezPlugins_ != nullptr && !reqpackBeezPlugins_->empty() &&
            !reqpackBeezPlugins_->find(Organization, Plugin).has_value())
        {
            throw std::runtime_error("configure_plugin references plugin '" + qualifiedName +
                                     "' which is not declared in reqpack.beez");
        }

        registry_->configurePlugin(Organization, Plugin, makeLuaStepConfig(LuaState, configTable));
    }

    void workflow(const std::string& name, const sol::table& steps) const
    {
        registry_->registerWorkflow(parseWorkflow(name, steps));
    }

    void workflow(const std::string& name, const std::string& pluginWorkflowReference) const
    {
        if (reqpackBeezPlugins_ != nullptr && !reqpackBeezPlugins_->empty())
        {
            const core::PluginWorkflowRef ParsedReference =
                core::parsePluginWorkflowReference(pluginWorkflowReference);
            if (!reqpackBeezPlugins_->find(ParsedReference.organization, ParsedReference.plugin)
                     .has_value())
            {
                throw std::runtime_error(
                    "workflow references plugin '" + ParsedReference.organization + '/' +
                    ParsedReference.plugin + "' which is not declared in reqpack.beez");
            }
        }

        registry_->registerWorkflowFromPluginReference(name, pluginWorkflowReference);
    }

    void workflows(const sol::table& workflowsTable) const
    {
        parseWorkflowsTable(workflowsTable, *registry_, reqpackBeezPlugins_);
    }

    void order(sol::variadic_args arguments) const
    {
        if (arguments.size() < 2U)
        {
            throw std::runtime_error("order() requires at least two step names");
        }

        std::vector<std::string> stepNames;
        stepNames.reserve(arguments.size());
        for (const sol::stack_proxy& argument : arguments)
        {
            if (!argument.is<std::string>())
            {
                throw std::runtime_error("order() arguments must be step name strings");
            }

            stepNames.push_back(argument.as<std::string>());
        }

        for (std::size_t index = 0; index + 1U < stepNames.size(); ++index)
        {
            registry_->registerStepOrder(stepNames.at(index), stepNames.at(index + 1U));
        }
    }

  private:
    core::Registry* registry_;
    std::weak_ptr<sol::state> luaState_;
    ReqpackBeezPluginCatalog* reqpackBeezPlugins_;
};

}  // namespace

void registerDsl(const std::shared_ptr<sol::state>& luaState,
                 core::Registry& registry,
                 const core::Context& context,
                 core::BeezSettings& buildSettings,
                 core::ReqPackManifest& reqpackManifest,
                 ReqpackBeezPluginCatalog& reqpackBeezPlugins)
{
    const std::weak_ptr<sol::state> WeakState = luaState;
    auto binder = std::make_shared<DslBinder>(&registry, WeakState, &reqpackBeezPlugins);

    (*luaState)["task"] = sol::overload(
        [binder](const std::string& name, const std::string& run) { binder->task(name, run); },
        [binder](const std::string& name, const sol::table& commands)
        { binder->task(name, commands); },
        [binder](const std::string& pluginTaskReference) { binder->task(pluginTaskReference); });

    (*luaState)["step"] = [binder](const sol::table& options) { binder->step(options); };

    (*luaState)["workflow"] =
        sol::overload([binder](const std::string& name, const std::string& pluginWorkflowReference)
                      { binder->workflow(name, pluginWorkflowReference); },
                      [binder](const std::string& name, const sol::table& steps)
                      { binder->workflow(name, steps); });

    (*luaState)["workflows"] = [binder](const sol::table& workflowsTable)
    { binder->workflows(workflowsTable); };

    (*luaState)["configure"] = [binder](const sol::table& entriesTable)
    { binder->configure(entriesTable); };

    (*luaState)["configure_step"] = [binder](const std::string& name, const sol::table& configTable)
    { binder->configureStep(name, configTable); };

    (*luaState)["configure_plugin"] =
        [binder](const std::string& qualifiedName, const sol::table& configTable)
    { binder->configurePlugin(qualifiedName, configTable); };

    (*luaState)["order"] = [binder](sol::variadic_args arguments) { binder->order(arguments); };

    (*luaState)["reqpack"] =
        [&reqpackManifest, &reqpackBeezPlugins, &registry, &context](const sol::table& table)
    {
        const sol::object BeezObject = table["beez"];
        if (BeezObject.valid() && BeezObject.is<sol::table>())
        {
            const auto Plugins = parseBeezPluginTable(BeezObject.as<sol::table>());
            reqpackBeezPlugins.merge(Plugins);
            for (const auto& pluginRef : Plugins)
            {
                if (pluginRef.version.has_value() && !pluginRef.isLocal())
                {
                    (void)tryLoadInstalledBeezPlugin(pluginRef.organization,
                                                     pluginRef.name,
                                                     *pluginRef.version,
                                                     registry,
                                                     context);
                }
            }
            loadBeezPlugins(Plugins, registry, context);
        }

        const auto ParsedManifest = parseReqPackTable(table);
        for (const auto& [plugin, packages] : ParsedManifest.plugins)
        {
            auto& target = reqpackManifest.plugins[plugin];
            target.insert(target.end(), packages.begin(), packages.end());
        }
    };

    const auto ProjectRoot = context.buildScriptPath().parent_path();
    const std::string ProjectPathPrefix =
        ProjectRoot.string() + "/?.lua;" + ProjectRoot.string() + "/?/init.lua";
    sol::table PackageTable = (*luaState)["package"];
    const std::string ExistingPath = PackageTable["path"];
    PackageTable["path"] = ProjectPathPrefix + ";" + ExistingPath;

    registerBeezApi(luaState, context, buildSettings);
}

}  // namespace beez::plugin::lua
// NOLINTEND(readability-identifier-naming)
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
