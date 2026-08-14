#include "beez/core/model/phase_scope_reference.hpp"

#include "beez/core/model/phase_invocation.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::optional<std::string> parseQuotedString(const std::string& input,
                                                           std::size_t& position)
{
    if (position >= input.size() || input.at(position) != '"')
    {
        return std::nullopt;
    }

    ++position;
    std::string value;
    while (position < input.size())
    {
        const char Character = input.at(position);
        if (Character == '"')
        {
            ++position;
            return value;
        }

        value.push_back(Character);
        ++position;
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<std::string> parseQuotedScopeList(const std::string& reference,
                                                              std::size_t bracketPosition)
{
    if (reference.back() != ']')
    {
        throw std::runtime_error("scoped reference '" + reference +
                                 "' must use the form 'name[scope]' or 'name[\"scope\"]'");
    }

    std::vector<std::string> scopes;
    std::size_t position = bracketPosition + 1U;
    while (position < reference.size())
    {
        while (position < reference.size() &&
               (reference.at(position) == ' ' || reference.at(position) == ','))
        {
            ++position;
        }

        if (position >= reference.size() || reference.at(position) == ']')
        {
            break;
        }

        const std::optional<std::string> Scope = parseQuotedString(reference, position);
        if (!Scope.has_value())
        {
            throw std::runtime_error("scoped reference '" + reference +
                                     "' has invalid bracket scope list");
        }

        scopes.push_back(*Scope);

        while (position < reference.size() && reference.at(position) == ' ')
        {
            ++position;
        }

        if (position < reference.size() && reference.at(position) == ',')
        {
            ++position;
            continue;
        }

        if (position < reference.size() && reference.at(position) == ']')
        {
            break;
        }

        throw std::runtime_error("scoped reference '" + reference +
                                 "' has invalid bracket scope list");
    }

    return scopes;
}

}  // namespace

PhaseInvocation parsePhaseScopeReference(const std::string& reference)
{
    const auto BracketPosition = reference.find('[');
    if (BracketPosition == std::string::npos || reference.size() < 3U || reference.back() != ']')
    {
        throw std::runtime_error("workflow phase reference '" + reference +
                                 "' must use the form 'phase[scope]'");
    }

    const std::string Phase = reference.substr(0, BracketPosition);
    const std::string Scope =
        reference.substr(BracketPosition + 1U, reference.size() - BracketPosition - 2U);

    if (Phase.empty() || Scope.empty())
    {
        throw std::runtime_error("workflow phase reference '" + reference +
                                 "' must use the form 'phase[scope]'");
    }

    return PhaseInvocation {.phase = Phase, .scope = Scope};
}

PhaseInvocation parsePhaseColonReference(const std::string& reference)
{
    const auto ColonPosition = reference.find(':');
    if (ColonPosition == std::string::npos || ColonPosition == 0U ||
        ColonPosition == reference.size() - 1U)
    {
        throw std::runtime_error("workflow phase reference '" + reference +
                                 "' must use the form 'phase:scope'");
    }

    const std::string Phase = reference.substr(0, ColonPosition);
    const std::string Scope = reference.substr(ColonPosition + 1U);

    if (Phase.empty() || Scope.empty())
    {
        throw std::runtime_error("workflow phase reference '" + reference +
                                 "' must use the form 'phase:scope'");
    }

    return PhaseInvocation {.phase = Phase, .scope = Scope};
}

PhaseInvocation parseWorkflowPhaseReference(const std::string& reference)
{
    if (reference.empty())
    {
        throw std::runtime_error("workflow phase reference must not be empty");
    }

    if (reference.find('[') != std::string::npos)
    {
        return parsePhaseScopeReference(reference);
    }

    if (reference.find(':') != std::string::npos)
    {
        return parsePhaseColonReference(reference);
    }

    return PhaseInvocation {.phase = reference, .scope = {}};
}

ScopedReference parseScopedReference(const std::string& reference)
{
    if (reference.empty())
    {
        throw std::runtime_error("scoped reference must not be empty");
    }

    const auto BracketPosition = reference.find('[');
    if (BracketPosition == std::string::npos)
    {
        return ScopedReference {.name = reference, .scopes = {}};
    }

    const std::string Name = reference.substr(0, BracketPosition);
    if (Name.empty() || reference.back() != ']')
    {
        throw std::runtime_error("scoped reference '" + reference +
                                 "' must use the form 'name[scope]' or 'name[\"scope\"]'");
    }

    const std::string Inner =
        reference.substr(BracketPosition + 1U, reference.size() - BracketPosition - 2U);
    if (Inner.empty())
    {
        throw std::runtime_error("scoped reference '" + reference + "' must include a scope");
    }

    if (Inner.front() == '"')
    {
        std::vector<std::string> Scopes = parseQuotedScopeList(reference, BracketPosition);
        if (Scopes.empty())
        {
            throw std::runtime_error("scoped reference '" + reference + "' must include a scope");
        }

        return ScopedReference {.name = Name, .scopes = std::move(Scopes)};
    }

    return ScopedReference {.name = Name, .scopes = {Inner}};
}

}  // namespace beez::core
