#include "beez/plugin/lua/lua_dsl.h"

#include "beez/core/context.h"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/step.hpp"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/plugin/plugin_host.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

struct LuaDslLoader::Impl
{
    std::shared_ptr<sol::state> luaState;
};

LuaDslLoader::LuaDslLoader() : impl_(std::make_unique<Impl>()) {}

LuaDslLoader::~LuaDslLoader() = default;

namespace
{

std::vector<std::string> parseCommandList(const sol::table& commandsTable)
{
    std::vector<std::string> commands;
    commandsTable.for_each(
        [&commands](const sol::object& /*key*/, const sol::object& value)
        {
            if (!value.is<std::string>())
            {
                throw std::runtime_error("task command list must contain only strings");
            }
            commands.push_back(value.as<std::string>());
        });

    if (commands.empty())
    {
        throw std::runtime_error("task command list must not be empty");
    }

    return commands;
}

bool isCommandListTable(const sol::table& table)
{
    if (table.empty())
    {
        return false;
    }

    bool hasStringEntry = false;
    table.for_each(
        [&hasStringEntry](const sol::object& key, const sol::object& value)
        {
            if (!key.is<int>())
            {
                return;
            }
            if (value.is<std::string>())
            {
                hasStringEntry = true;
            }
        });

    return hasStringEntry;
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
        step.callback = [luaState, LuaFunction](const core::Context& /*context*/) mutable -> int
        {
            const sol::protected_function_result Result = LuaFunction();
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
    core::PhaseInvocation invocation;
    invocation.phase = invocationTable["phase"].get<std::string>();
    invocation.scope = invocationTable["scope"].get<std::string>();
    return invocation;
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
        [&workflow](const sol::object& /*key*/, const sol::object& value)
        {
            if (!value.is<sol::table>())
            {
                return;
            }
            workflow.steps.push_back(parseWorkflowStep(value.as<sol::table>()));
        });

    return workflow;
}

class DslBinder
{
  public:
    DslBinder(core::Registry* registry, std::shared_ptr<sol::state> luaState)
        : registry_(registry), luaState_(std::move(luaState))
    {
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- DSL binding mirrors Lua call order
    void task(const std::string& name, const std::string& run) const
    {
        core::Task task;
        task.name = name;
        task.commands = {run};
        registry_->registerTask(std::move(task));
    }

    void task(const std::string& name, const sol::table& commands) const
    {
        if (!isCommandListTable(commands))
        {
            throw std::runtime_error("task '" + name + "' table form must be a list of commands");
        }

        core::Task task;
        task.name = name;
        task.commands = parseCommandList(commands);
        registry_->registerTask(std::move(task));
    }

    void step(const sol::table& options) const
    {
        registry_->registerStep(parseStepTable(options, luaState_));
    }

    void workflow(const std::string& name, const sol::table& steps) const
    {
        registry_->registerWorkflow(parseWorkflow(name, steps));
    }

  private:
    core::Registry* registry_;
    std::shared_ptr<sol::state> luaState_;
};

void registerDsl(const std::shared_ptr<sol::state>& luaState, core::Registry& registry)
{
    auto binder = std::make_shared<DslBinder>(&registry, luaState);

    (*luaState)["task"] = sol::overload(
        [binder](const std::string& name, const std::string& run) { binder->task(name, run); },
        [binder](const std::string& name, const sol::table& commands)
        { binder->task(name, commands); });

    (*luaState)["step"] = [binder](const sol::table& options) { binder->step(options); };

    (*luaState)["workflow"] = [binder](const std::string& name, const sol::table& steps)
    { binder->workflow(name, steps); };
}

}  // namespace

bool LuaDslLoader::load(const core::Context& context, core::Registry& registry)
{
    try
    {
        impl_->luaState = std::make_shared<sol::state>();
        impl_->luaState->open_libraries(sol::lib::base, sol::lib::package);

        registerDsl(impl_->luaState, registry);

        const auto ScriptPath = context.buildScriptPath().string();
        impl_->luaState->script_file(ScriptPath);
        return true;
    }
    catch (const sol::error& error)
    {
        std::cerr << "Lua error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        return false;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DSL error: " << error.what() << '\n';
        impl_->luaState = nullptr;
        return false;
    }
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
