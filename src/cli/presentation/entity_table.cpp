#include "beez/cli/presentation/entity_table.hpp"

#include "beez/core/registry/registry.hpp"
#include "beez/core/util/text_table.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace beez::cli
{

namespace
{

[[nodiscard]] std::string joinStrings(const std::vector<std::string>& parts,
                                      const std::string& separator)
{
    if (parts.empty())
    {
        return {};
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
        if (index > 0)
        {
            stream << separator;
        }

        stream << parts.at(index);
    }

    return stream.str();
}

[[nodiscard]] std::string formatOptionalDescription(const std::optional<std::string>& description)
{
    if (!description.has_value() || description->empty())
    {
        return "-";
    }

    return *description;
}

[[nodiscard]] std::string formatBracketedList(const std::vector<std::string>& values)
{
    if (values.empty())
    {
        return "[]";
    }

    return '[' + joinStrings(values, ", ") + ']';
}

[[nodiscard]] std::unordered_map<std::string, std::vector<std::string>>
scopesByPhase(const core::Registry& registry)
{
    std::unordered_map<std::string, std::vector<std::string>> scopes;
    for (const auto& [name, step] : registry.steps())
    {
        (void)name;
        if (step.phase.empty())
        {
            continue;
        }

        auto& phaseScopes = scopes[step.phase];
        if (step.scope.empty())
        {
            continue;
        }

        if (std::ranges::find(phaseScopes, step.scope) == phaseScopes.end())
        {
            phaseScopes.push_back(step.scope);
        }
    }

    for (auto& [phase, phaseScopes] : scopes)
    {
        (void)phase;
        std::ranges::sort(phaseScopes);
    }

    return scopes;
}

[[nodiscard]] std::string formatNameTable(const std::vector<std::string>& names)
{
    core::TextTable table({"Name"});
    for (const auto& name : names)
    {
        table.addRow({name});
    }

    return table.format();
}

[[nodiscard]] std::string formatStepTable(const core::Registry& registry,
                                          const std::vector<std::string>& names)
{
    core::TextTable table({"Name", "Phase", "Scope", "Description"});
    for (const auto& name : names)
    {
        const auto Resolved = registry.resolveStep(name);
        if (!Resolved.hasValue())
        {
            continue;
        }

        const auto& step = Resolved.value();
        table.addRow({name,
                      step.phase.empty() ? "-" : step.phase,
                      step.scope.empty() ? "-" : step.scope,
                      formatOptionalDescription(step.description)});
    }

    return table.format();
}

[[nodiscard]] std::string formatPhaseTable(const core::Registry& registry,
                                           const std::vector<std::string>& names)
{
    const auto ScopesByPhase = scopesByPhase(registry);
    core::TextTable table({"Phase", "Scopes"});
    for (const auto& phase : names)
    {
        const auto Iterator = ScopesByPhase.find(phase);
        const std::string Scopes =
            Iterator == ScopesByPhase.end() ? "[]" : formatBracketedList(Iterator->second);
        table.addRow({phase, Scopes});
    }

    return table.format();
}

}  // namespace

std::vector<std::string> collectEntityNames(const core::Registry& registry, const std::string& kind)
{
    std::vector<std::string> names;
    if (kind == "tasks")
    {
        names.reserve(registry.tasks().size());
        for (const auto& [name, task] : registry.tasks())
        {
            (void)task;
            names.push_back(name);
        }
    }
    else if (kind == "workflows")
    {
        names.reserve(registry.workflows().size());
        for (const auto& [name, workflow] : registry.workflows())
        {
            (void)workflow;
            names.push_back(name);
        }
    }
    else if (kind == "steps")
    {
        names = registry.stepInvocationNames();
    }
    else if (kind == "phases")
    {
        names = registry.phases();
    }

    std::ranges::sort(names);
    return names;
}

std::string formatEntityList(const core::Registry& registry, const std::string& kind)
{
    const auto Names = collectEntityNames(registry, kind);

    std::ostringstream stream;
    stream << kind << ":\n\n";

    if (kind == "tasks" || kind == "workflows")
    {
        stream << formatNameTable(Names);
    }
    else if (kind == "steps")
    {
        stream << formatStepTable(registry, Names);
    }
    else if (kind == "phases")
    {
        stream << formatPhaseTable(registry, Names);
    }

    stream << '\n';
    return stream.str();
}

}  // namespace beez::cli
