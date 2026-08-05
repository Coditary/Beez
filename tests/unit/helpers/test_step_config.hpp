#pragma once

#include "beez/core/step_config.hpp"

#include <memory>
#include <string>
#include <utility>

namespace beez::test
{

class TestStepConfig final : public core::StepConfig
{
  public:
    explicit TestStepConfig(std::string tag) : tag_(std::move(tag)) {}

    [[nodiscard]] bool empty() const override
    {
        return tag_.empty();
    }

    [[nodiscard]] const std::string& tag() const
    {
        return tag_;
    }

    [[nodiscard]] core::StepConfigPtr mergedWith(const core::StepConfigPtr& overlay) const override
    {
        const auto* overlayConfig = dynamic_cast<const TestStepConfig*>(overlay.get());
        if (overlayConfig == nullptr)
        {
            return overlay;
        }

        if (tag_.empty())
        {
            return overlay;
        }

        if (overlayConfig->tag_.empty())
        {
            return std::make_shared<TestStepConfig>(tag_);
        }

        return std::make_shared<TestStepConfig>(tag_ + "+" + overlayConfig->tag_);
    }

  private:
    std::string tag_;
};

inline core::StepConfigPtr makeTestConfig(const std::string& tag)
{
    return std::make_shared<TestStepConfig>(tag);
}

}  // namespace beez::test
