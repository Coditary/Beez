#pragma once

#include "beez/core/execution/concurrency/worker_pool.hpp"

#include <memory>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <sol/forward.hpp>
// NOLINTEND(misc-include-cleaner)

namespace beez::plugin::lua
{

struct WorkerWaitOptions
{
    bool exitCode = false;
    bool output = false;
    bool duration = false;
    bool cached = false;
    bool name = false;
    bool id = false;
    bool dryRun = false;

    [[nodiscard]] bool wantsResult() const
    {
        return exitCode || output || duration || cached || name || id || dryRun;
    }
};

[[nodiscard]] core::WorkerSpec parseWorkerSpec(const sol::table& options);

[[nodiscard]] WorkerWaitOptions parseWorkerWaitOptions(const sol::table& options);

[[nodiscard]] core::WorkerHandle parseWorkerHandleFromObject(const sol::object& handleValue);

[[nodiscard]] std::vector<core::WorkerHandle> parseWorkerHandleList(const sol::table& handlesTable);

[[nodiscard]] sol::object buildWorkerWaitResult(const std::shared_ptr<sol::state>& luaState,
                                                const core::WorkerSnapshot& snapshot,
                                                const WorkerWaitOptions& options);

}  // namespace beez::plugin::lua
