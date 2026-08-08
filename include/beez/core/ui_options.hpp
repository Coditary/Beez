#pragma once

#include "beez/logging/logger.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace beez::core
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
inline constexpr std::uint32_t DefaultIndicatorSpinIntervalMs = 80U;

enum class ProgressDisplayStyle : std::uint8_t
{
    Minimal,
    Lines,
    Blocks,
    Custom,
};

enum class ProgressIndicatorStyle : std::uint8_t
{
    Step,
    Percent,
    SpinnerMinimal,
    SpinnerDots,
    SpinnerCustom,
};

enum class UiLogLevel : std::uint8_t
{
    Info,
    Warn,
    Error,
};

struct CustomProgressStyle
{
    std::string startDelimiter = "[";
    std::string endDelimiter = "]";
    std::string fillChar;
    std::string emptyChar;
};

struct CustomIndicatorStyle
{
    std::string startDelimiter;
    std::string endDelimiter;
};

struct ProgressDisplaySettings
{
    ProgressDisplayStyle style = ProgressDisplayStyle::Minimal;
    ProgressIndicatorStyle indicator = ProgressIndicatorStyle::Step;
    CustomProgressStyle custom;
    CustomIndicatorStyle indicatorStyle;
    std::vector<std::string> customSpinnerFrames;
    std::uint32_t indicatorSpinIntervalMs = DefaultIndicatorSpinIntervalMs;
};

struct UiAnimationSettings
{
    ProgressDisplaySettings progress;
};

struct UiColorPalette
{
    std::string text;
    std::string muted;
    std::string success;
    std::string warning;
    std::string error;
    std::string info;
    std::string accent;
    std::string progressFill;
    std::string progressEmpty;
    std::string cacheHit;
    std::string workerPrefix;
};

struct UiAnimationOverlay
{
    std::optional<std::string> progress;
    std::optional<CustomProgressStyle> customProgress;
    std::optional<std::string> indicator;
    std::optional<std::vector<std::string>> customIndicatorFrames;
    std::optional<std::string> indicatorStartDelimiter;
    std::optional<std::string> indicatorEndDelimiter;
    std::optional<std::uint32_t> indicatorSpinIntervalMs;
    std::optional<std::string> spinner;
    std::optional<std::vector<std::string>> customSpinnerFrames;
};

struct UiSettingsOverlay
{
    std::optional<bool> colors;
    std::optional<bool> truecolor;
    std::optional<std::string> theme;
    std::optional<std::map<std::string, UiColorPalette>> themes;
    std::optional<bool> icons;
    std::optional<UiAnimationOverlay> animation;
    std::optional<std::string> logLevel;
    std::optional<bool> hideCacheHits;
    std::optional<bool> workerPrefix;
    std::optional<std::string> workerPrefixFormat;
    std::optional<bool> showTimeSaved;
};

struct UiSettings
{
    bool colors = true;
    bool truecolor = true;
    UiColorPalette palette;
    bool icons = true;
    UiAnimationSettings animation;
    UiLogLevel logLevel = UiLogLevel::Info;
    bool hideCacheHits = false;
    bool workerPrefixEnabled = false;
    std::string workerPrefixFormat = "[Worker {id}]";
    bool showTimeSaved = true;
};

[[nodiscard]] ProgressDisplayStyle parseProgressDisplayStyle(const std::string& value);
[[nodiscard]] ProgressIndicatorStyle parseProgressIndicatorStyle(const std::string& value);
[[nodiscard]] UiLogLevel parseUiLogLevel(const std::string& value);

[[nodiscard]] const char* toString(ProgressDisplayStyle style);
[[nodiscard]] const char* toString(ProgressIndicatorStyle indicator);
[[nodiscard]] const char* toString(UiLogLevel level);

[[nodiscard]] std::vector<const char*> progressDisplayStyleNames();
[[nodiscard]] std::vector<const char*> progressIndicatorStyleNames();
[[nodiscard]] std::vector<const char*> uiLogLevelNames();

[[nodiscard]] bool isSpinnerIndicatorStyle(ProgressIndicatorStyle indicator);
[[nodiscard]] bool isBarProgressStyle(ProgressDisplayStyle style);
[[nodiscard]] bool usesAnimatedProgressSpinner(const UiSettings& settings);

[[nodiscard]] UiColorPalette
resolveThemePalette(const std::optional<std::string>& themeName,
                    const std::map<std::string, UiColorPalette>& themes);

[[nodiscard]] UiSettings resolveUiSettings(const UiSettingsOverlay& overlay);
void mergeUiSettingsOverlay(UiSettingsOverlay& target, const UiSettingsOverlay& overlay);

[[nodiscard]] std::string
colorizeText(const UiSettings& settings, const std::string& hexColor, std::string_view text);

[[nodiscard]] std::string
formatProgressLine(const UiSettings& settings,
                   const logging::ExecutionProgress& progress,
                   bool cached = false,
                   std::optional<std::size_t> spinnerFrame = std::nullopt);

[[nodiscard]] std::string formatWorkerOutputPrefix(const UiSettings& settings,
                                                   std::uint64_t channelId,
                                                   std::string_view channelLabel);

[[nodiscard]] std::string formatRunSummaryLine(const UiSettings& settings,
                                               const logging::RunSummary& summary);

}  // namespace beez::core
