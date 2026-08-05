#include "beez/core/step_config.hpp"

namespace beez::core
{

StepConfigPtr mergeStepConfigs(const StepConfigPtr& base, const StepConfigPtr& overlay)
{
    if (overlay == nullptr || overlay->empty())
    {
        return base;
    }

    if (base == nullptr || base->empty())
    {
        return overlay;
    }

    return base->mergedWith(overlay);
}

}  // namespace beez::core
