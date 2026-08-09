#pragma once

#include <memory>
#include <string>

namespace beez::core
{

class StepConfig
{
  public:
    StepConfig() = default;
    virtual ~StepConfig() = default;

    StepConfig(const StepConfig&) = delete;
    StepConfig& operator=(const StepConfig&) = delete;
    StepConfig(StepConfig&&) = delete;
    StepConfig& operator=(StepConfig&&) = delete;

    [[nodiscard]] virtual bool empty() const = 0;

    [[nodiscard]] virtual std::string cacheFingerprint() const = 0;

    [[nodiscard]] virtual std::shared_ptr<const StepConfig>
    mergedWith(const std::shared_ptr<const StepConfig>& overlay) const = 0;
};

using StepConfigPtr = std::shared_ptr<const StepConfig>;

[[nodiscard]] StepConfigPtr mergeStepConfigs(const StepConfigPtr& base,
                                             const StepConfigPtr& overlay);

}  // namespace beez::core
