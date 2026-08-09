#include "beez/plugin/lua/lua_dsl.h"

#include "beez/core/context.h"
#include "beez/core/env_file.hpp"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/task_action.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/plugin/plugin_host.h"
#include "lua_step_config.hpp"

#include <cstdlib>
#include <iostream>
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

void mergeSettingsFromLuaTable(const sol::table& table, core::BeezSettings& settings);

struct LuaDslLoader::Impl
{
    std::shared_ptr<sol::state> luaState;
    core::BeezSettings buildSettings;
};

LuaDslLoader::LuaDslLoader() : impl_(std::make_unique<Impl>()) {}

LuaDslLoader::~LuaDslLoader() = default;

namespace
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

core::PhaseInvocation parsePhaseInvocation(const sol::table& invocationTable)
{
    const sol::object PhaseValue = invocationTable["phase"];
    if (!PhaseValue.valid() || !PhaseValue.is<std::string>() ||
        PhaseValue.as<std::string>().empty())
    {
        throw std::runtime_error("workflow phase invocation is missing required field 'phase'");
    }

    const sol::object ScopeValue = invocationTable["scope"];
    if (!ScopeValue.valid() || !ScopeValue.is<std::string>() ||
        ScopeValue.as<std::string>().empty())
    {
        throw std::runtime_error("workflow phase invocation is missing required field 'scope'");
    }

    return core::PhaseInvocation {.phase = PhaseValue.as<std::string>(),
                                  .scope = ScopeValue.as<std::string>()};
}

core::WorkflowStep parseWorkflowStep(const sol::table& stepTable)
{
    const sol::object ParallelValue = stepTable["parallel"];
    if (ParallelValue.valid() && ParallelValue.is<sol::table>())
    {
        core::WorkflowStep step;

        const sol::table ParallelTable = ParallelValue.as<sol::table>();
        ParallelTable.for_each(
            [&step](const sol::object& /*key*/, const sol::object& value)
            {
                if (!value.is<sol::table>())
                {
                    return;
                }
                step.invocations.push_back(parsePhaseInvocation(value.as<sol::table>()));
            });

        if (step.invocations.empty())
        {
            throw std::runtime_error("workflow parallel step requires at least one phase");
        }

        return step;
    }

    return core::WorkflowStep {.invocations = {parsePhaseInvocation(stepTable)}};
}

core::Workflow parseWorkflow(const std::string& name, const sol::table& stepsTable)
{
    core::Workflow workflow;
    workflow.name = name;

    stepsTable.for_each(
        [&workflow, &name](const sol::object& /*key*/, const sol::object& value)
        {
            if (!value.is<sol::table>())
            {
                if (value.is<std::string>())
                {
                    throw std::runtime_error("workflow '" + name +
                                             "' entry must be a phase table, not a string ('" +
                                             value.as<std::string>() + "')");
                }

                if (value.is<int>() || value.is<double>())
                {
                    throw std::runtime_error("workflow '" + name +
                                             "' entry must be a phase table, not a number");
                }

                return;
            }

            workflow.steps.push_back(parseWorkflowStep(value.as<sol::table>()));
        });

    return workflow;
}

void validateLoadedRegistry(core::Registry& registry)
{
    for (const auto& [taskName, task] : registry.tasks())
    {
        for (const auto& action : task.actions)
        {
            if (const auto* stepAction = std::get_if<core::TaskStepAction>(&action))
            {
                if (!registry.findStep(stepAction->stepName).has_value())
                {
                    throw std::runtime_error("task '" + taskName + "' references undefined step '" +
                                             stepAction->stepName + "'");
                }
            }
        }
    }

    for (const auto& [workflowName, workflow] : registry.workflows())
    {
        for (const auto& workflowStep : workflow.steps)
        {
            for (const auto& invocation : workflowStep.invocations)
            {
                const auto Matched = registry.stepsForPhase(invocation.phase, invocation.scope);
                if (!Matched.hasValue())
                {
                    throw std::runtime_error(
                        "workflow '" + workflowName + "' step ordering failed for phase '" +
                        invocation.phase + "' scope '" + invocation.scope + "'");
                }

                if (Matched.value().empty())
                {
                    throw std::runtime_error(
                        "workflow '" + workflowName + "' has no registered steps for phase '" +
                        invocation.phase + "' scope '" + invocation.scope + "'");
                }
            }
        }
    }

    registry.validateConsistent();
}

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

}  // namespace

bool LuaDslLoader::load(const core::Context& context, core::Registry& registry)
{
    registry.clear();
    try
    {
        impl_->buildSettings = {};
        impl_->luaState = nullptr;
        impl_->luaState = std::make_shared<sol::state>();
        impl_->luaState->open_libraries(sol::lib::base, sol::lib::package);

        registerDsl(impl_->luaState, registry, context, impl_->buildSettings);

        const auto ScriptPath = context.buildScriptPath().string();
        impl_->luaState->script_file(ScriptPath);
        validateLoadedRegistry(registry);
        return true;
    }
    catch (const sol::error& error)
    {
        std::cerr << "Lua error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        registry.clear();
        return false;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DSL error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        registry.clear();
        return false;
    }
}

const core::BeezSettings& LuaDslLoader::buildSettings() const
{
    return impl_->buildSettings;
}

void LuaDslLoader::setGcThroughputMode(bool enable)
{
    if (impl_->luaState == nullptr)
    {
        return;
    }

    sol::state_view view(*impl_->luaState);
    if (enable)
    {
        view.stop_gc();
        return;
    }

    view.restart_gc();
    view.collect_garbage();
}

void LuaDslLoader::releaseState()
{
    impl_->luaState = nullptr;
}

std::string LuaDslPlugin::name() const
{
    return "lua_dsl";
}

void LuaDslPlugin::registerCapabilities(PluginHost& host)
{
    host.setDslLoader(std::make_unique<LuaDslLoader>());
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
