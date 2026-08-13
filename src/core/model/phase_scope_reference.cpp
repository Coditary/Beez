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

}  // namespace beez::core
