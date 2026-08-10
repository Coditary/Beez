#pragma once

#include "beez/core/cache/step/types.hpp"
#include "beez/core/model/step.hpp"
#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/types.hpp"
#include "beez/core/util/expected.hpp"

#include <string>

namespace beez::core
{

class Orchestrator;

void recordStepCacheSkip(Orchestrator& orchestrator,
                         const Step& step,
                         const CacheLookupResult& lookup,
                         ProgressState& progress,
                         const std::string& category,
                         const std::string& detail);

namespace step_callback_detail
{
Expected<int, OrchestratorError> run(Orchestrator& orchestrator, const Step& step);
}  // namespace step_callback_detail

}  // namespace beez::core
