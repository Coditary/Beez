// NOLINTBEGIN(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
#include "beez/core/config/ui/resolve.hpp"

#include "beez/core/config/ui/theme.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::string defaultIndicatorStartDelimiter(ProgressIndicatorStyle indicator)
{
    switch (indicator)
    {
    case ProgressIndicatorStyle::SpinnerDots:
    case ProgressIndicatorStyle::SpinnerCustom:
        return {};
    case ProgressIndicatorStyle::Step:
    case ProgressIndicatorStyle::Percent:
    case ProgressIndicatorStyle::SpinnerMinimal:
        break;
    }

    return "[";
}

[[nodiscard]] std::string defaultIndicatorEndDelimiter(ProgressIndicatorStyle indicator)
{
    switch (indicator)
    {
    case ProgressIndicatorStyle::SpinnerDots:
    case ProgressIndicatorStyle::SpinnerCustom:
        return {};
    case ProgressIndicatorStyle::Step:
    case ProgressIndicatorStyle::Percent:
    case ProgressIndicatorStyle::SpinnerMinimal:
        break;
    }

    return "]";
}

void applyAnimationOverlay(const UiAnimationOverlay& animation, UiSettings& settings)
{
    if (animation.progress.has_value())
    {
        settings.animation.progress.style = parseProgressDisplayStyle(*animation.progress);
    }
    if (animation.customProgress.has_value())
    {
        settings.animation.progress.style = ProgressDisplayStyle::Custom;
        settings.animation.progress.custom = *animation.customProgress;
    }

    bool indicatorConfigured = false;
    if (animation.indicator.has_value())
    {
        settings.animation.progress.indicator = parseProgressIndicatorStyle(*animation.indicator);
        indicatorConfigured = true;
    }
    if (animation.customIndicatorFrames.has_value())
    {
        settings.animation.progress.indicator = ProgressIndicatorStyle::SpinnerCustom;
        settings.animation.progress.customSpinnerFrames = *animation.customIndicatorFrames;
        indicatorConfigured = true;
    }

    if (!indicatorConfigured)
    {
        if (animation.spinner.has_value())
        {
            settings.animation.progress.indicator = parseProgressIndicatorStyle(*animation.spinner);
        }
        if (animation.customSpinnerFrames.has_value())
        {
            settings.animation.progress.indicator = ProgressIndicatorStyle::SpinnerCustom;
            settings.animation.progress.customSpinnerFrames = *animation.customSpinnerFrames;
        }
    }
    else if (animation.customSpinnerFrames.has_value() &&
             settings.animation.progress.customSpinnerFrames.empty())
    {
        settings.animation.progress.customSpinnerFrames = *animation.customSpinnerFrames;
    }

    if (animation.indicatorStartDelimiter.has_value())
    {
        settings.animation.progress.indicatorStyle.startDelimiter =
            *animation.indicatorStartDelimiter;
    }
    if (animation.indicatorEndDelimiter.has_value())
    {
        settings.animation.progress.indicatorStyle.endDelimiter = *animation.indicatorEndDelimiter;
    }
    if (animation.indicatorSpinIntervalMs.has_value())
    {
        settings.animation.progress.indicatorSpinIntervalMs = *animation.indicatorSpinIntervalMs;
    }
}

}  // namespace

ProgressDisplayStyle parseProgressDisplayStyle(const std::string& value)
{
    if (value == "lines")
    {
        return ProgressDisplayStyle::Lines;
    }
    if (value == "blocks")
    {
        return ProgressDisplayStyle::Blocks;
    }
    if (value == "custom")
    {
        return ProgressDisplayStyle::Custom;
    }
    if (value == "minimal")
    {
        return ProgressDisplayStyle::Minimal;
    }

    throw std::runtime_error(
        "ui.animation.progress must be 'minimal', 'lines', 'blocks', or 'custom'");
}

ProgressIndicatorStyle parseProgressIndicatorStyle(const std::string& value)
{
    if (value == "percent" || value == "percentage")
    {
        return ProgressIndicatorStyle::Percent;
    }
    if (value == "minimal")
    {
        return ProgressIndicatorStyle::SpinnerMinimal;
    }
    if (value == "dots")
    {
        return ProgressIndicatorStyle::SpinnerDots;
    }
    if (value == "custom")
    {
        return ProgressIndicatorStyle::SpinnerCustom;
    }
    if (value == "step" || value == "fraction")
    {
        return ProgressIndicatorStyle::Step;
    }

    throw std::runtime_error(
        "ui.animation.indicator must be 'step', 'percent', 'minimal', 'dots', or 'custom'");
}

UiLogLevel parseUiLogLevel(const std::string& value)
{
    if (value == "warn")
    {
        return UiLogLevel::Warn;
    }
    if (value == "error")
    {
        return UiLogLevel::Error;
    }
    if (value == "info")
    {
        return UiLogLevel::Info;
    }

    throw std::runtime_error("ui.log_level must be 'info', 'warn', or 'error'");
}

RunSummaryStyle parseRunSummaryStyle(const std::string& value)
{
    if (value == "minimal")
    {
        return RunSummaryStyle::Minimal;
    }

    if (value == "simple")
    {
        return RunSummaryStyle::Simple;
    }

    if (value == "compact")
    {
        return RunSummaryStyle::Compact;
    }

    if (value == "data")
    {
        return RunSummaryStyle::Data;
    }

    throw std::runtime_error("ui.summary must be 'minimal', 'simple', 'compact', or 'data'");
}

const char* toString(const ProgressDisplayStyle style)
{
    switch (style)
    {
    case ProgressDisplayStyle::Lines:
        return "lines";
    case ProgressDisplayStyle::Blocks:
        return "blocks";
    case ProgressDisplayStyle::Custom:
        return "custom";
    case ProgressDisplayStyle::Minimal:
        return "minimal";
    }

    return "minimal";
}

const char* toString(const ProgressIndicatorStyle indicator)
{
    switch (indicator)
    {
    case ProgressIndicatorStyle::Percent:
        return "percent";
    case ProgressIndicatorStyle::SpinnerDots:
        return "dots";
    case ProgressIndicatorStyle::SpinnerCustom:
        return "custom";
    case ProgressIndicatorStyle::SpinnerMinimal:
        return "minimal";
    case ProgressIndicatorStyle::Step:
        return "step";
    }

    return "step";
}

const char* toString(const UiLogLevel level)
{
    switch (level)
    {
    case UiLogLevel::Warn:
        return "warn";
    case UiLogLevel::Error:
        return "error";
    case UiLogLevel::Info:
        return "info";
    }

    return "info";
}

const char* toString(const RunSummaryStyle style)
{
    switch (style)
    {
    case RunSummaryStyle::Minimal:
        return "minimal";
    case RunSummaryStyle::Simple:
        return "simple";
    case RunSummaryStyle::Compact:
        return "compact";
    case RunSummaryStyle::Data:
        return "data";
    }

    return "simple";
}

std::vector<const char*> progressDisplayStyleNames()
{
    return {"minimal", "lines", "blocks", "custom"};
}

std::vector<const char*> progressIndicatorStyleNames()
{
    return {"step", "percent", "minimal", "dots", "custom"};
}

std::vector<const char*> uiLogLevelNames()
{
    return {"info", "warn", "error"};
}

std::vector<const char*> runSummaryStyleNames()
{
    return {"minimal", "simple", "compact", "data"};
}

bool isSpinnerIndicatorStyle(const ProgressIndicatorStyle indicator)
{
    switch (indicator)
    {
    case ProgressIndicatorStyle::SpinnerMinimal:
    case ProgressIndicatorStyle::SpinnerDots:
    case ProgressIndicatorStyle::SpinnerCustom:
        return true;
    case ProgressIndicatorStyle::Step:
    case ProgressIndicatorStyle::Percent:
        break;
    }

    return false;
}

bool isBarProgressStyle(const ProgressDisplayStyle style)
{
    switch (style)
    {
    case ProgressDisplayStyle::Lines:
    case ProgressDisplayStyle::Blocks:
    case ProgressDisplayStyle::Custom:
        return true;
    case ProgressDisplayStyle::Minimal:
        break;
    }

    return false;
}

bool usesAnimatedProgressSpinner(const UiSettings& settings)
{
    const ProgressDisplaySettings& progress = settings.animation.progress;
    return isSpinnerIndicatorStyle(progress.indicator) && progress.indicatorSpinIntervalMs > 0;
}

UiSettings resolveUiSettings(const UiSettingsOverlay& overlay)
{
    UiSettings settings;
    settings.colors = overlay.colors.value_or(true);
    if (overlay.truecolor.has_value())
    {
        settings.truecolor = *overlay.truecolor;
    }
    else
    {
        settings.truecolor = settings.colors;
    }

    settings.icons = overlay.icons.value_or(true);
    settings.logLevel =
        overlay.logLevel.has_value() ? parseUiLogLevel(*overlay.logLevel) : UiLogLevel::Info;
    settings.hideCacheHits = overlay.hideCacheHits.value_or(false);
    settings.workerPrefixEnabled = overlay.workerPrefix.value_or(false);
    settings.workerPrefixFormat = overlay.workerPrefixFormat.value_or("[Worker {id}]");
    settings.showTimeSaved = overlay.showTimeSaved.value_or(true);
    settings.summaryStyle = overlay.summary.has_value() ? parseRunSummaryStyle(*overlay.summary)
                                                        : RunSummaryStyle::Simple;

    if (overlay.theme.has_value())
    {
        settings.palette = resolveThemePalette(
            overlay.theme, overlay.themes.value_or(std::map<std::string, UiColorPalette> {}));
    }
    else if (settings.colors)
    {
        settings.palette = defaultColorPalette();
    }

    if (overlay.animation.has_value())
    {
        applyAnimationOverlay(*overlay.animation, settings);
    }

    if (!settings.icons &&
        settings.animation.progress.indicator == ProgressIndicatorStyle::SpinnerDots)
    {
        settings.animation.progress.indicator = ProgressIndicatorStyle::SpinnerMinimal;
    }

    const bool HasExplicitIndicatorStart =
        overlay.animation.has_value() && overlay.animation->indicatorStartDelimiter.has_value();
    const bool HasExplicitIndicatorEnd =
        overlay.animation.has_value() && overlay.animation->indicatorEndDelimiter.has_value();

    if (!HasExplicitIndicatorStart)
    {
        settings.animation.progress.indicatorStyle.startDelimiter =
            defaultIndicatorStartDelimiter(settings.animation.progress.indicator);
    }
    if (!HasExplicitIndicatorEnd)
    {
        settings.animation.progress.indicatorStyle.endDelimiter =
            defaultIndicatorEndDelimiter(settings.animation.progress.indicator);
    }

    return settings;
}

void mergeUiSettingsOverlay(UiSettingsOverlay& target, const UiSettingsOverlay& overlay)
{
    if (overlay.colors.has_value())
    {
        target.colors = overlay.colors;
    }
    if (overlay.truecolor.has_value())
    {
        target.truecolor = overlay.truecolor;
    }
    if (overlay.theme.has_value())
    {
        target.theme = overlay.theme;
    }
    if (overlay.themes.has_value())
    {
        if (!target.themes.has_value())
        {
            target.themes = overlay.themes;
        }
        else
        {
            for (const auto& [name, palette] : *overlay.themes)
            {
                (*target.themes)[name] = palette;
            }
        }
    }
    if (overlay.icons.has_value())
    {
        target.icons = overlay.icons;
    }
    if (overlay.animation.has_value())
    {
        target.animation = overlay.animation;
    }
    if (overlay.logLevel.has_value())
    {
        target.logLevel = overlay.logLevel;
    }
    if (overlay.hideCacheHits.has_value())
    {
        target.hideCacheHits = overlay.hideCacheHits;
    }
    if (overlay.workerPrefix.has_value())
    {
        target.workerPrefix = overlay.workerPrefix;
    }
    if (overlay.workerPrefixFormat.has_value())
    {
        target.workerPrefixFormat = overlay.workerPrefixFormat;
    }
    if (overlay.showTimeSaved.has_value())
    {
        target.showTimeSaved = overlay.showTimeSaved;
    }
    if (overlay.summary.has_value())
    {
        target.summary = overlay.summary;
    }
    if (overlay.logging.has_value())
    {
        if (!target.logging.has_value())
        {
            target.logging = overlay.logging;
        }
        else
        {
            logging::mergeLoggingSettingsOverlay(*target.logging, *overlay.logging);
        }
    }
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
