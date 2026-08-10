#pragma once

#include "beez/logging/settings/logging_settings.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
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

enum class RunSummaryStyle : std::uint8_t
{
    Minimal,
    Simple,
    Compact,
    Data,
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
    std::optional<std::string> summary;
    std::optional<logging::LoggingSettingsOverlay> logging;
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
    RunSummaryStyle summaryStyle = RunSummaryStyle::Simple;
};

}  // namespace beez::core
