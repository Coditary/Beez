#include "beez/plugin/lua/lua_dsl.h"

#include "beez/core/context.h"
#include "beez/core/phase_invocation.hpp"
#include "beez/core/registry.h"
#include "beez/core/task.hpp"
#include "beez/core/workflow.hpp"
#include "beez/core/workflow_step.hpp"
#include "beez/plugin/plugin_host.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

core::Task parseTaskTable(const std::string& name, const sol::table& options)
{
    core::Task task;
    task.name = name;

    const sol::object RunValue = options["run"];
    if (!RunValue.valid() || !RunValue.is<std::string>())
    {
        throw std::runtime_error("task '" + name + "' is missing required field 'run'");
    }
    task.run = RunValue.as<std::string>();

    const sol::object PhaseValue = options["phase"];
    if (PhaseValue.valid() && PhaseValue.is<std::string>())
    {
        task.phase = PhaseValue.as<std::string>();
    }

    const sol::object ScopeValue = options["scope"];
    if (ScopeValue.valid() && ScopeValue.is<std::string>())
    {
        task.scope = ScopeValue.as<std::string>();
    }

    return task;
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
    explicit DslBinder(core::Registry* registry) : registry_(registry) {}

    void task(const std::string& name, const std::string& run) const
    {
        core::Task task;
        task.name = name;
        task.run = run;
        registry_->registerTask(std::move(task));
    }

    void task(const std::string& name, const sol::table& options) const
    {
        registry_->registerTask(parseTaskTable(name, options));
    }

    void workflow(const std::string& name, const sol::table& steps) const
    {
        registry_->registerWorkflow(parseWorkflow(name, steps));
    }

  private:
    core::Registry* registry_;
};

void registerDsl(sol::state& lua, core::Registry& registry)
{
    auto binder = std::make_shared<DslBinder>(&registry);

    lua["task"] = sol::overload([binder](const std::string& name, const std::string& run)
                                { binder->task(name, run); },
                                [binder](const std::string& name, const sol::table& options)
                                { binder->task(name, options); });

    lua["workflow"] = [binder](const std::string& name, const sol::table& steps)
    { binder->workflow(name, steps); };
}

}  // namespace

bool LuaDslLoader::load(const core::Context& context, core::Registry& registry)
{
    try
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::package);

        registerDsl(lua, registry);

        const auto ScriptPath = context.buildScriptPath().string();
        lua.script_file(ScriptPath);
        return true;
    }
    catch (const sol::error& error)
    {
        std::cerr << "Lua error: " << error.what() << '\n';
        return false;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DSL error: " << error.what() << '\n';
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
// NOLINTEND(misc-include-cleaner)
