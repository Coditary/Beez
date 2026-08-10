#pragma once

#include "beez/core/config/ui/types.hpp"

#include <string>
#include <vector>

namespace beez::core
{

[[nodiscard]] ProgressDisplayStyle parseProgressDisplayStyle(const std::string& value);
[[nodiscard]] ProgressIndicatorStyle parseProgressIndicatorStyle(const std::string& value);
[[nodiscard]] UiLogLevel parseUiLogLevel(const std::string& value);
[[nodiscard]] RunSummaryStyle parseRunSummaryStyle(const std::string& value);

[[nodiscard]] const char* toString(ProgressDisplayStyle style);
[[nodiscard]] const char* toString(ProgressIndicatorStyle indicator);
[[nodiscard]] const char* toString(UiLogLevel level);
[[nodiscard]] const char* toString(RunSummaryStyle style);

[[nodiscard]] std::vector<const char*> progressDisplayStyleNames();
[[nodiscard]] std::vector<const char*> progressIndicatorStyleNames();
[[nodiscard]] std::vector<const char*> uiLogLevelNames();
[[nodiscard]] std::vector<const char*> runSummaryStyleNames();

[[nodiscard]] bool isSpinnerIndicatorStyle(ProgressIndicatorStyle indicator);
[[nodiscard]] bool isBarProgressStyle(ProgressDisplayStyle style);
[[nodiscard]] bool usesAnimatedProgressSpinner(const UiSettings& settings);

[[nodiscard]] UiSettings resolveUiSettings(const UiSettingsOverlay& overlay);
void mergeUiSettingsOverlay(UiSettingsOverlay& target, const UiSettingsOverlay& overlay);

}  // namespace beez::core
