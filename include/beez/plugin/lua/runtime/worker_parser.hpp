#pragma once

#include "beez/core/execution/worker_pool.hpp"

#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

[[nodiscard]] core::WorkerSpec parseWorkerSpec(const sol::table& options);

[[nodiscard]] core::WorkerHandle parseWorkerHandleFromObject(const sol::object& handleValue);

[[nodiscard]] std::vector<core::WorkerHandle> parseWorkerHandleList(const sol::table& handlesTable);

}  // namespace beez::plugin::lua
