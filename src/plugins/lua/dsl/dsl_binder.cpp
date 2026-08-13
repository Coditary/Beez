#include "beez/plugin/lua/dsl/dsl_binder.hpp"

#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/plugin/lua/api/beez_table.hpp"
#include "beez/plugin/lua/dsl/plugin_loader.hpp"
#include "beez/plugin/lua/dsl/reqpack_parser.hpp"
#include "beez/plugin/lua/dsl/step_parser.hpp"
#include "beez/plugin/lua/dsl/task_parser.hpp"
#include "beez/plugin/lua/dsl/workflow_parser.hpp"
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

class DslBinder
{
  public:
    DslBinder(core::Registry* registry, std::weak_ptr<sol::state> luaState)
        : registry_(registry), luaState_(std::move(luaState))
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

    void step(const sol::table& options) const
    {
        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        registry_->registerStep(parseStepTable(options, LuaState));
    }

    void configureStep(const std::string& name, const sol::table& configTable) const
    {
        const auto LuaState = luaState_.lock();
        if (!LuaState)
        {
            throw std::runtime_error("lua state is no longer available");
        }

        registry_->configureStep(name, makeLuaStepConfig(LuaState, configTable));
    }

    void workflow(const std::string& name, const sol::table& steps) const
    {
        registry_->registerWorkflow(parseWorkflow(name, steps));
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
};

}  // namespace

void registerDsl(const std::shared_ptr<sol::state>& luaState,
                 core::Registry& registry,
                 const core::Context& context,
                 core::BeezSettings& buildSettings,
                 core::ReqPackManifest& reqpackManifest)
{
    const std::weak_ptr<sol::state> WeakState = luaState;
    auto binder = std::make_shared<DslBinder>(&registry, WeakState);

    (*luaState)["task"] = sol::overload(
        [binder](const std::string& name, const std::string& run) { binder->task(name, run); },
        [binder](const std::string& name, const sol::table& commands)
        { binder->task(name, commands); });

    (*luaState)["step"] = [binder](const sol::table& options) { binder->step(options); };

    (*luaState)["configure_step"] = [binder](const std::string& name, const sol::table& configTable)
    { binder->configureStep(name, configTable); };

    (*luaState)["workflow"] = [binder](const std::string& name, const sol::table& steps)
    { binder->workflow(name, steps); };

    (*luaState)["order"] = [binder](sol::variadic_args arguments) { binder->order(arguments); };

    (*luaState)["reqpack"] = [&reqpackManifest, &registry, &context](const sol::table& table)
    {
        const sol::object BeezObject = table["beez"];
        if (BeezObject.valid() && BeezObject.is<sol::table>())
        {
            loadBeezPlugins(parseBeezPluginTable(BeezObject.as<sol::table>()), registry, context);
        }

        reqpackManifest = parseReqPackTable(table);
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
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
