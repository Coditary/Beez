#include "beez/plugin/lua/dsl/task_step_reference.hpp"

#include "beez/core/model/phase_scope_reference.hpp"
#include "beez/core/registry/step_reference.hpp"

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

[[nodiscard]] std::string readStepFieldValue(const sol::table& stepTable)
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

    const std::string StepName = StepValue.as<std::string>();
    if (StepName.empty())
    {
        throw std::runtime_error("task action field 'step' must not be empty");
    }

    if (StepName.find('@') != std::string::npos)
    {
        throw std::runtime_error(
            "task action field 'step' must not include a version suffix; use reqpack.beez");
    }

    return StepName;
}

[[nodiscard]] std::vector<std::string>
buildStepNamesFromScopedReference(const core::ScopedReference& scoped)
{
    if (scoped.scopes.empty())
    {
        return {scoped.name};
    }

    std::vector<std::string> stepNames;
    stepNames.reserve(scoped.scopes.size());
    for (const std::string& Scope : scoped.scopes)
    {
        if (scoped.name.find(':') != std::string::npos)
        {
            throw std::runtime_error(
                "task action field 'step' cannot combine bracket scopes with a scoped step name");
        }

        stepNames.push_back(scoped.name + ':' + Scope);
    }

    return stepNames;
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

    const sol::object ScopeValue = stepTable["scope"];
    if (ScopeValue.valid() && ScopeValue.get_type() != sol::type::lua_nil)
    {
        throw std::runtime_error(
            "task action field 'scope' is no longer supported; use step[name[scope]] or "
            "phase[name[scope]] syntax");
    }
}

std::vector<std::string> parseTaskStepReferences(const sol::table& stepTable)
{
    rejectDeprecatedTaskFields(stepTable);

    const sol::object PluginValue = stepTable["plugin"];
    const std::vector<std::string> StepNames =
        buildStepNamesFromScopedReference(core::parseScopedReference(readStepFieldValue(stepTable)));

    std::vector<std::string> references;
    references.reserve(StepNames.size());

    if (PluginValue.valid() && PluginValue.get_type() != sol::type::lua_nil)
    {
        if (!PluginValue.is<std::string>())
        {
            throw std::runtime_error("task action field 'plugin' must be a string");
        }

        const auto [Organization, Plugin] = splitQualifiedPluginName(PluginValue.as<std::string>());
        for (const std::string& StepName : StepNames)
        {
            references.push_back(beez::core::formatQualifiedStepRef(Organization, Plugin, StepName));
        }

        return references;
    }

    for (const std::string& StepName : StepNames)
    {
        if (StepName.find('/') != std::string::npos)
        {
            throw std::runtime_error(
                "task plugin steps require field 'plugin'; local steps must not use qualified names");
        }

        references.push_back(StepName);
    }

    return references;
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
