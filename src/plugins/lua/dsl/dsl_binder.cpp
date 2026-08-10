#include "beez/plugin/lua/dsl/dsl_binder.hpp"

#include "beez/core/env/env_file.hpp"
#include "beez/core/model/task.hpp"
#include "beez/core/model/task_action.hpp"
#include "beez/plugin/lua/dsl/step_parser.hpp"
#include "beez/plugin/lua/dsl/task_parser.hpp"
#include "beez/plugin/lua/dsl/workflow_parser.hpp"
#include "beez/plugin/lua/runtime/step_config.hpp"
#include "beez/plugin/lua/settings/settings_overlay.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

class BeezDslEnv
{
  public:
    explicit BeezDslEnv(const core::Context& context) : envFilePath_(context.envFilePath()) {}

    sol::object env(sol::this_state lua, const std::string& key) const
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c) -- process env lookup for build DSL
        if (const char* processValue = std::getenv(key.c_str()))
        {
            return sol::make_object(lua, std::string(processValue));
        }

        if (!envFile_.has_value())
        {
            envFile_.emplace(envFilePath_);
        }

        const auto Value = envFile_->lookup(key);
        if (!Value.has_value())
        {
            return sol::lua_nil;
        }

        return sol::make_object(lua, *Value);
    }

  private:
    std::filesystem::path envFilePath_;
    mutable std::optional<core::EnvFile> envFile_;
};

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

    void order(const std::string& before, const std::string& after) const
    {
        registry_->registerStepOrder(before, after);
    }

  private:
    core::Registry* registry_;
    std::weak_ptr<sol::state> luaState_;
};

}  // namespace

void registerDsl(const std::shared_ptr<sol::state>& luaState,
                 core::Registry& registry,
                 const core::Context& context,
                 core::BeezSettings& buildSettings)
{
    const std::weak_ptr<sol::state> WeakState = luaState;
    auto binder = std::make_shared<DslBinder>(&registry, WeakState);
    auto beezApi = std::make_shared<BeezDslEnv>(context);

    (*luaState)["task"] = sol::overload(
        [binder](const std::string& name, const std::string& run) { binder->task(name, run); },
        [binder](const std::string& name, const sol::table& commands)
        { binder->task(name, commands); });

    (*luaState)["step"] = [binder](const sol::table& options) { binder->step(options); };

    (*luaState)["configure_step"] = [binder](const std::string& name, const sol::table& configTable)
    { binder->configureStep(name, configTable); };

    (*luaState)["workflow"] = [binder](const std::string& name, const sol::table& steps)
    { binder->workflow(name, steps); };

    (*luaState)["order"] = [binder](const std::string& before, const std::string& after)
    { binder->order(before, after); };

    sol::table beezTable = luaState->create_table();
    beezTable["env"] = [beezApi](sol::this_state lua, const std::string& key)
    { return beezApi->env(lua, key); };
    beezTable["config"] = [&buildSettings, &context](const sol::object& options)
    {
        if (!options.is<sol::table>())
        {
            throw std::runtime_error("beez.config argument must be a table");
        }

        mergeSettingsFromLuaTable(options.as<sol::table>(), buildSettings);
        buildSettings.applyEnvironment(context);
    };
    (*luaState)["beez"] = beezTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
