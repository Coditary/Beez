#include "beez/plugin/lua/dsl/task_step_reference.hpp"

#include "beez/core/registry/step_reference.hpp"

#include <stdexcept>
#include <string>
#include <utility>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

namespace
{

[[nodiscard]] std::pair<std::string, std::string> splitQualifiedPluginName(const std::string& name)
{
    const auto SlashPosition = name.find('/');
    if (SlashPosition == std::string::npos || SlashPosition == 0 || SlashPosition == name.size() - 1)
    {
        throw std::runtime_error("task plugin field '" + name +
                                 "' must use the form 'organization/plugin'");
    }

    return {name.substr(0, SlashPosition), name.substr(SlashPosition + 1)};
}

[[nodiscard]] std::string readStepField(const sol::table& stepTable)
{
    const sol::object StepValue = stepTable["step"];
    if (!StepValue.valid() || StepValue.get_type() == sol::type::lua_nil)
    {
        throw std::runtime_error("task action is missing required field 'step'");
    }

    if (!StepValue.is<std::string>())
    {
        throw std::runtime_error("task action field 'step' must be a string");
    }

    std::string stepName = StepValue.as<std::string>();
    if (stepName.empty())
    {
        throw std::runtime_error("task action field 'step' must not be empty");
    }

    if (stepName.find('@') != std::string::npos)
    {
        throw std::runtime_error(
            "task action field 'step' must not include a version suffix; use reqpack.beez");
    }

    const sol::object ScopeValue = stepTable["scope"];
    if (ScopeValue.valid() && ScopeValue.get_type() != sol::type::lua_nil)
    {
        if (!ScopeValue.is<std::string>())
        {
            throw std::runtime_error("task action field 'scope' must be a string");
        }

        if (stepName.find(':') != std::string::npos)
        {
            throw std::runtime_error(
                "task action field 'scope' cannot be combined with a scoped step name");
        }

        stepName += ':';
        stepName += ScopeValue.as<std::string>();
    }

    return stepName;
}

[[nodiscard]] bool isPluginStepRegistrationId(const std::string& stepId)
{
    return stepId.find('/') != std::string::npos;
}

}  // namespace

void rejectDeprecatedTaskFields(const sol::table& stepTable)
{
    const sol::object NameValue = stepTable["name"];
    if (NameValue.valid() && NameValue.get_type() != sol::type::lua_nil)
    {
        throw std::runtime_error(
            "task action field 'name' is no longer supported; use 'step' or 'task'");
    }

    const sol::object VersionValue = stepTable["version"];
    if (VersionValue.valid() && VersionValue.get_type() != sol::type::lua_nil)
    {
        throw std::runtime_error(
            "task action field 'version' is not supported; declare plugin versions in reqpack.beez");
    }
}

std::string parseTaskStepReference(const sol::table& stepTable)
{
    rejectDeprecatedTaskFields(stepTable);

    const sol::object PluginValue = stepTable["plugin"];
    const std::string StepName = readStepField(stepTable);

    if (PluginValue.valid() && PluginValue.get_type() != sol::type::lua_nil)
    {
        if (!PluginValue.is<std::string>())
        {
            throw std::runtime_error("task action field 'plugin' must be a string");
        }

        const auto [Organization, Plugin] = splitQualifiedPluginName(PluginValue.as<std::string>());
        return beez::core::formatQualifiedStepRef(Organization, Plugin, StepName);
    }

    if (StepName.find('/') != std::string::npos)
    {
        throw std::runtime_error(
            "task plugin steps require field 'plugin'; local steps must not use qualified names");
    }

    return StepName;
}

void validateTaskPluginStepReference(const std::string& taskName,
                                     const std::string& stepReference,
                                     const core::Registry& registry,
                                     const ReqpackBeezPluginCatalog& catalog)
{
    if (stepReference.find('@') != std::string::npos)
    {
        throw std::runtime_error("task '" + taskName + "' plugin step '" + stepReference +
                                 "' must not include a version suffix; use reqpack.beez");
    }

    if (const auto Qualified = beez::core::parseQualifiedStepRef(stepReference))
    {
        const auto CatalogEntry = catalog.find(Qualified->organization, Qualified->plugin);
        if (!CatalogEntry.has_value())
        {
            throw std::runtime_error("task '" + taskName + "' references plugin '" +
                                     Qualified->organization + '/' + Qualified->plugin +
                                     "' which is not declared in reqpack.beez");
        }

        return;
    }

    const auto RegistrationId = registry.resolveStepRegistrationId(stepReference);
    if (!RegistrationId.hasValue())
    {
        return;
    }

    if (!isPluginStepRegistrationId(RegistrationId.value()))
    {
        return;
    }

    throw std::runtime_error("task '" + taskName + "' plugin step '" + stepReference +
                             "' requires fields 'plugin' and 'step'");
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
