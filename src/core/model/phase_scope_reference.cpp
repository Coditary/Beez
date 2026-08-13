#include "beez/core/model/phase_scope_reference.hpp"

#include <stdexcept>
#include <string>

namespace beez::core
{

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
    if (reference.find('[') != std::string::npos)
    {
        return parsePhaseScopeReference(reference);
    }

    return parsePhaseColonReference(reference);
}

}  // namespace beez::core
