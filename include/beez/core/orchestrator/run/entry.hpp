#pragma once

#include "beez/core/orchestrator/errors.hpp"
#include "beez/core/orchestrator/run/lifecycle.hpp"
#include "beez/core/util/expected.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace beez::core
{

class Orchestrator;

namespace orchestrator_detail
{

enum class RunCacheFlushPolicy : std::uint8_t
{
    Never,
    IfEndStrategy,
    AtRunEnd,
};

[[nodiscard]] LoggedRunScope
beginLoggedRun(Orchestrator& orchestrator, const std::string& runType, const std::string& name);

class ScopedLoggedRun
{
  public:
    ScopedLoggedRun(Orchestrator& orchestrator,
                    const std::string& runType,
                    const std::string& name,
                    RunCacheFlushPolicy flushPolicy);

    [[nodiscard]] LoggedRunScope& scope()
    {
        return scope_;
    }

    void finish(bool success);

    template <typename RunFn>
    [[nodiscard]] Expected<int, OrchestratorError> withSegment(const std::string& label,
                                                               RunFn&& runFn)
    {
        scope_.beginSegment(label);
        const auto Result = std::forward<RunFn>(runFn)();
        scope_.endSegment(static_cast<bool>(Result));
        finish(static_cast<bool>(Result));
        return Result;
    }

    template <typename RunFn>
    [[nodiscard]] Expected<int, OrchestratorError> withoutSegment(RunFn&& runFn)
    {
        const auto Result = std::forward<RunFn>(runFn)();
        finish(static_cast<bool>(Result));
        return Result;
    }

  private:
    void flushCache();

    Orchestrator& orchestrator_;
    LoggedRunScope scope_;
    RunCacheFlushPolicy flushPolicy_;
    bool finished_ = false;
};

}  // namespace orchestrator_detail
}  // namespace beez::core
