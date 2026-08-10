#pragma once

#include "beez/core/orchestrator/errors.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>

namespace beez::core
{

struct ProgressState
{
    std::atomic<std::size_t> index {0};
    std::size_t total = 0;
};

struct WorkflowExecutionState
{
    std::atomic<bool> failed {false};
    OrchestratorError error = OrchestratorError::ExecutionFailed;
    std::mutex errorMutex;
};

struct ProgressLabel
{
    std::string category;
    std::string detail;
};

struct ActiveRunSegment
{
    std::string label;
    std::chrono::steady_clock::time_point started;
    std::size_t steps = 0;
    std::size_t cacheHits = 0;
};

}  // namespace beez::core
