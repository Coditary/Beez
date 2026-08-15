#pragma once

#include "beez/core/model/phase_invocation.hpp"

#include <string>
#include <vector>

namespace beez::core
{

struct ScopedReference
{
    std::string name;
    std::vector<std::string> scopes;
};

[[nodiscard]] PhaseInvocation parsePhaseScopeReference(const std::string& reference);
[[nodiscard]] PhaseInvocation parsePhaseColonReference(const std::string& reference);
[[nodiscard]] PhaseInvocation parseWorkflowPhaseReference(const std::string& reference);
[[nodiscard]] ScopedReference parseScopedReference(const std::string& reference);

}  // namespace beez::core
