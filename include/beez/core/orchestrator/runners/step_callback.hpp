#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/util/expected.hpp"

namespace beez::core
{

class Orchestrator;
class Step;

namespace step_callback_detail
{

Expected<int, OrchestratorError> run(Orchestrator& orchestrator, const Step& step);

}  // namespace step_callback_detail
}  // namespace beez::core
