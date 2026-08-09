// NOLINTBEGIN(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
#include "beez/core/ui_options.hpp"

#include "beez/logging/contract/run_types.hpp"
#include "beez/logging/settings/logging_settings.hpp"

#include "beez/core/util/text_table.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace beez::core
{

namespace
{

constexpr std::array<const char*, 4> MinimalSpinnerFrames = {"|", "/", "-", "\\"};
constexpr std::array<const char*, 8> DotsSpinnerFrames = {"⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"};

[[nodiscard]] bool isHexDigit(const char Character)
{
    return std::isxdigit(static_cast<unsigned char>(Character)) != 0;
}

[[nodiscard]] std::optional<std::uint8_t> parseHexByte(std::string_view text)
{
    if (text.size() != 2 || !isHexDigit(text[0]) || !isHexDigit(text[1]))
    {
        return std::nullopt;
    }

    const auto ParseNibble = [](const char Character) -> std::uint8_t
    {
        if (Character >= '0' && Character <= '9')
        {
            return static_cast<std::uint8_t>(Character - '0');
        }

        const char Lower = static_cast<char>(std::tolower(static_cast<unsigned char>(Character)));
        return static_cast<std::uint8_t>(Lower - 'a' + 10);
    };

    return static_cast<std::uint8_t>((ParseNibble(text[0]) << 4U) | ParseNibble(text[1]));
}

[[nodiscard]] std::optional<std::array<std::uint8_t, 3>> parseHexColor(std::string_view value)
{
    if (value.empty())
    {
        return std::nullopt;
    }

    if (value.front() == '#')
    {
        value.remove_prefix(1);
    }

    if (value.size() != 6)
    {
        return std::nullopt;
    }

    const auto Red = parseHexByte(value.substr(0, 2));
    const auto Green = parseHexByte(value.substr(2, 2));
    const auto Blue = parseHexByte(value.substr(4, 2));
    if (!Red.has_value() || !Green.has_value() || !Blue.has_value())
    {
        return std::nullopt;
    }

    return std::array<std::uint8_t, 3> {*Red, *Green, *Blue};
}

[[nodiscard]] std::string trueColorSequence(const std::array<std::uint8_t, 3>& rgb)
{
    return "\033[38;2;" + std::to_string(rgb[0]) + ';' + std::to_string(rgb[1]) + ';' +
           std::to_string(rgb[2]) + 'm';
}

[[nodiscard]] std::string basicColorSequence(const std::array<std::uint8_t, 3>& rgb)
{
    const int Luminance = (299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]) / 1000;
    if (Luminance >= 180)
    {
        return "\033[97m";
    }
    if (Luminance >= 120)
    {
        return "\033[37m";
    }
    if (Luminance >= 80)
    {
        return "\033[90m";
    }

    if (rgb[0] > rgb[1] && rgb[0] > rgb[2])
    {
        return "\033[31m";
    }
    if (rgb[1] > rgb[0] && rgb[1] > rgb[2])
    {
        return "\033[32m";
    }
    if (rgb[2] > rgb[0] && rgb[2] > rgb[1])
    {
        return "\033[34m";
    }

    return "\033[36m";
}

[[nodiscard]] std::string ansiForeground(const UiSettings& settings, const std::string& hexColor)
{
    const auto Rgb = parseHexColor(hexColor);
    if (!Rgb.has_value())
    {
        return {};
    }

    if (settings.truecolor)
    {
        return trueColorSequence(*Rgb);
    }

    return basicColorSequence(*Rgb);
}

[[nodiscard]] std::string
replaceToken(std::string text, std::string_view token, std::string_view replacement)
{
    std::size_t position = 0;
    while ((position = text.find(token, position)) != std::string::npos)
    {
        text.replace(position, token.size(), replacement);
        position += replacement.size();
    }

    return text;
}

[[nodiscard]] std::string formatStepIndicator(std::size_t index, std::size_t total)
{
    return std::to_string(index) + '/' + std::to_string(total);
}

[[nodiscard]] std::string formatPercentIndicator(std::size_t index, std::size_t total)
{
    if (total == 0)
    {
        return "0%";
    }

    return std::to_string(static_cast<std::size_t>(
               std::lround((static_cast<double>(index) * 100.0) / static_cast<double>(total)))) +
           '%';
}

[[nodiscard]] std::string formatSpinnerIndicator(const ProgressDisplaySettings& progress,
                                                 std::size_t frameIndex)
{
    switch (progress.indicator)
    {
    case ProgressIndicatorStyle::SpinnerDots:
        return DotsSpinnerFrames.at(frameIndex % DotsSpinnerFrames.size());
    case ProgressIndicatorStyle::SpinnerCustom:
        if (!progress.customSpinnerFrames.empty())
        {
            return progress.customSpinnerFrames.at(frameIndex %
                                                   progress.customSpinnerFrames.size());
        }
        break;
    case ProgressIndicatorStyle::SpinnerMinimal:
    case ProgressIndicatorStyle::Step:
    case ProgressIndicatorStyle::Percent:
        break;
    }

    return MinimalSpinnerFrames.at(frameIndex % MinimalSpinnerFrames.size());
}

[[nodiscard]] std::string formatProgressIndicator(const ProgressDisplaySettings& progress,
                                                  std::size_t index,
                                                  std::size_t total,
                                                  const std::optional<std::size_t> spinnerFrame)
{
    switch (progress.indicator)
    {
    case ProgressIndicatorStyle::Percent:
        return formatPercentIndicator(index, total);
    case ProgressIndicatorStyle::SpinnerMinimal:
    case ProgressIndicatorStyle::SpinnerDots:
    case ProgressIndicatorStyle::SpinnerCustom:
        return formatSpinnerIndicator(progress, spinnerFrame.value_or(index > 0 ? index - 1 : 0));
    case ProgressIndicatorStyle::Step:
        break;
    }

    return formatStepIndicator(index, total);
}

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

[[nodiscard]] std::string formatIndicatorSegment(const UiSettings& settings,
                                                 const std::string& indicatorText)
{
    const auto& indicatorStyle = settings.animation.progress.indicatorStyle;
    return colorizeText(settings,
                        settings.palette.muted,
                        indicatorStyle.startDelimiter + indicatorText +
                            indicatorStyle.endDelimiter);
}

[[nodiscard]] std::string defaultFillGlyph(bool useBlocks, const std::string& configured)
{
    if (!configured.empty())
    {
        return configured;
    }

    return useBlocks ? "█" : "=";
}

[[nodiscard]] std::string defaultEmptyGlyph(bool useBlocks, const std::string& configured)
{
    if (!configured.empty())
    {
        return configured;
    }

    return useBlocks ? "░" : "-";
}

[[nodiscard]] std::string renderBar(const CustomProgressStyle& style,
                                    std::size_t index,
                                    std::size_t total,
                                    std::size_t width,
                                    bool useBlocks)
{
    if (total == 0)
    {
        const std::string EmptyGlyph = defaultEmptyGlyph(useBlocks, style.emptyChar);
        std::string bar;
        for (std::size_t character = 0; character < width; ++character)
        {
            bar += EmptyGlyph;
        }
        return bar;
    }

    const std::size_t Filled = std::min(width, (index * width + total - 1) / total);
    const std::string FillGlyph = defaultFillGlyph(useBlocks, style.fillChar);
    const std::string EmptyGlyph = defaultEmptyGlyph(useBlocks, style.emptyChar);

    std::string bar;
    for (std::size_t character = 0; character < Filled; ++character)
    {
        bar += FillGlyph;
    }
    for (std::size_t character = Filled; character < width; ++character)
    {
        bar += EmptyGlyph;
    }

    return bar;
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

[[nodiscard]] UiColorPalette defaultColorPalette()
{
    return UiColorPalette {
        .text = "#d4d4d4",
        .muted = "#808080",
        .success = "#33cc33",
        .warning = "#e6c200",
        .error = "#e05252",
        .info = "#4da3ff",
        .accent = "#2ec4c4",
        .progressFill = "#33cc33",
        .progressEmpty = "#555555",
        .cacheHit = "#6fbf6f",
        .workerPrefix = "#5aa9ff",
    };
}

[[nodiscard]] UiColorPalette
resolveThemePalette(const std::optional<std::string>& themeName,
                    const std::map<std::string, UiColorPalette>& themes)
{
    if (!themeName.has_value())
    {
        return {};
    }

    const auto Found = themes.find(*themeName);
    if (Found == themes.end())
    {
        throw std::runtime_error("unknown ui theme: " + *themeName);
    }

    return Found->second;
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

std::string
colorizeText(const UiSettings& settings, const std::string& hexColor, const std::string_view text)
{
    if (!settings.colors || text.empty())
    {
        return std::string(text);
    }

    const std::string Sequence = ansiForeground(settings, hexColor);
    if (Sequence.empty())
    {
        return std::string(text);
    }

    return Sequence + std::string(text) + "\033[0m";
}

std::string formatProgressLine(const UiSettings& settings,
                               const logging::ExecutionProgress& progress,
                               const bool cached,
                               const std::optional<std::size_t> spinnerFrame)
{
    const auto Indicator = formatProgressIndicator(
        settings.animation.progress, progress.index, progress.total, spinnerFrame);
    const std::string IndicatorSegment = formatIndicatorSegment(settings, Indicator);
    const std::string Category = cached ? progress.category + " (cached)" : progress.category;
    const std::string& accent = cached ? settings.palette.cacheHit : settings.palette.accent;
    const std::string ColoredCategory = colorizeText(settings, accent, Category);
    const std::string ColoredDetail =
        colorizeText(settings, settings.palette.text, progress.detail);

    switch (settings.animation.progress.style)
    {
    case ProgressDisplayStyle::Minimal:
        return IndicatorSegment + ' ' + ColoredCategory + " | " + ColoredDetail;
    case ProgressDisplayStyle::Lines:
    case ProgressDisplayStyle::Blocks:
    case ProgressDisplayStyle::Custom:
    {
        const bool UseBlocks = settings.animation.progress.style == ProgressDisplayStyle::Blocks;
        const auto& custom = settings.animation.progress.custom;
        const std::string Bar = renderBar(custom, progress.index, progress.total, 20, UseBlocks);
        const std::string ColoredBar = colorizeText(settings, settings.palette.progressFill, Bar);
        return custom.startDelimiter + ColoredBar + custom.endDelimiter + ' ' + IndicatorSegment +
               ' ' + ColoredCategory + " | " + ColoredDetail;
    }
    }

    return ColoredCategory + " | " + ColoredDetail;
}

std::string formatWorkerOutputPrefix(const UiSettings& settings,
                                     const std::uint64_t ChannelId,
                                     const std::string_view ChannelLabel)
{
    if (!settings.workerPrefixEnabled)
    {
        return "  | ";
    }

    const std::string Identifier =
        ChannelLabel.empty() ? std::to_string(ChannelId) : std::string(ChannelLabel);
    const std::string Prefix = replaceToken(settings.workerPrefixFormat, "{id}", Identifier);
    return colorizeText(settings, settings.palette.workerPrefix, Prefix) + ' ';
}

std::string formatRunSummaryLine(const UiSettings& settings, const logging::RunSummary& summary)
{
    if (settings.summaryStyle != RunSummaryStyle::Minimal || !settings.showTimeSaved ||
        summary.cacheHitsSkipped == 0)
    {
        return {};
    }

    const std::string Message = "Saved time on " + std::to_string(summary.cacheHitsSkipped) +
                                (summary.cacheHitsSkipped == 1 ? " cache hit" : " cache hits");
    return colorizeText(settings, settings.palette.cacheHit, Message);
}

namespace
{

constexpr std::size_t CompactLabelColumnWidth = 8;
constexpr std::size_t CompactBoxMinInnerWidth = 32;
constexpr std::string_view RunEndSeparator =
    "============================================================";

[[nodiscard]] std::string formatCompactRow(const std::string& label, const std::string& value)
{
    std::string row = label;
    if (row.size() < CompactLabelColumnWidth)
    {
        row.append(CompactLabelColumnWidth - row.size(), ' ');
    }

    row += value;
    return row;
}

[[nodiscard]] std::string formatCompactBoxBorder(const std::string_view left,
                                                 const std::string_view fill,
                                                 const std::string_view right,
                                                 const std::size_t innerWidth)
{
    std::string border;
    border.reserve(left.size() + right.size() + (innerWidth + 2U) * fill.size());
    border.append(left);
    for (std::size_t index = 0; index < innerWidth + 2U; ++index)
    {
        border.append(fill);
    }

    border.append(right);
    return border;
}

[[nodiscard]] std::size_t maxContentWidth(const std::vector<std::string>& rows)
{
    if (rows.empty())
    {
        return 0U;
    }

    const auto WidestRow = std::ranges::max_element(rows, std::ranges::less {}, &std::string::size);
    return WidestRow == rows.end() ? 0U : WidestRow->size();
}

[[nodiscard]] std::optional<std::string_view>
findFailedSegmentName(const logging::RunSummary& summary)
{
    const auto FailedSegment = std::ranges::find_if(
        summary.segments, [](const logging::SegmentSummary& segment) { return !segment.success; });
    if (FailedSegment == summary.segments.end())
    {
        return std::nullopt;
    }

    return FailedSegment->name;
}

[[nodiscard]] std::string formatDurationShort(const double seconds)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << seconds << 's';
    return stream.str();
}

[[nodiscard]] std::string formatDurationApprox(const double seconds)
{
    if (seconds <= 0.0)
    {
        return "~0s";
    }

    const auto TotalSeconds = static_cast<std::size_t>(std::lround(seconds));
    if (TotalSeconds < 60U)
    {
        return "~" + formatDurationShort(seconds);
    }

    const std::size_t Minutes = TotalSeconds / 60U;
    const std::size_t RemainingSeconds = TotalSeconds % 60U;
    if (Minutes < 60U)
    {
        return "~" + std::to_string(Minutes) + "m " + std::to_string(RemainingSeconds) + 's';
    }

    const std::size_t Hours = Minutes / 60U;
    const std::size_t RemainingMinutes = Minutes % 60U;
    return "~" + std::to_string(Hours) + "h " + std::to_string(RemainingMinutes) + 'm';
}

[[nodiscard]] std::string formatCompactTimeValue(const bool success, const double durationSeconds)
{
    const std::string Duration = formatDurationShort(durationSeconds);
    if (success)
    {
        return "finished in " + Duration;
    }

    return "failed after " + Duration;
}

[[nodiscard]] std::size_t cacheHitPercent(const std::size_t hits, const std::size_t total)
{
    if (total == 0U)
    {
        return 0U;
    }

    return (hits * 100U) / total;
}

[[nodiscard]] std::string formatCacheStats(const std::size_t hits, const std::size_t total)
{
    return std::to_string(hits) + '/' + std::to_string(total) + " cached (" +
           std::to_string(cacheHitPercent(hits, total)) + "%)";
}

[[nodiscard]] std::string formatWorkerCount(const std::size_t workers)
{
    return std::to_string(workers) + (workers == 1U ? " worker" : " workers");
}

[[nodiscard]] std::size_t displayWorkerCount(const logging::RunSummary& summary)
{
    if (summary.peakWorkers > 0U)
    {
        return summary.peakWorkers;
    }

    return summary.workerThreads;
}

[[nodiscard]] std::string formatSavedSuffix(const UiSettings& settings,
                                            const logging::RunSummary& summary)
{
    if (!settings.showTimeSaved || summary.estimatedTimeSavedSeconds <= 0.0)
    {
        return {};
    }

    return " | saved " + formatDurationApprox(summary.estimatedTimeSavedSeconds);
}

[[nodiscard]] std::string formatSimpleRunEnd(const UiSettings& settings,
                                             const bool success,
                                             const double durationSeconds,
                                             const logging::RunSummary& summary)
{
    const std::string Duration = formatDurationShort(durationSeconds);
    const std::string CacheStats = formatCacheStats(summary.cacheHitsSkipped, summary.totalSteps);
    const std::string Workers = formatWorkerCount(displayWorkerCount(summary));

    std::ostringstream stream;
    if (settings.icons)
    {
        stream << (success ? "✓ " : "✗ ");
    }

    if (success)
    {
        stream << "Build finished in " << Duration;
    }
    else
    {
        stream << "Build failed after " << Duration;
    }

    stream << " | " << CacheStats << " | " << Workers << formatSavedSuffix(settings, summary);
    return stream.str();
}

[[nodiscard]] std::string formatCompactBoxLine(const UiSettings& settings,
                                               const std::string_view visibleContent,
                                               const std::size_t innerWidth,
                                               const std::string& hexColor = {})
{
    std::string content;
    if (!hexColor.empty() && settings.colors)
    {
        content = colorizeText(settings, hexColor, visibleContent);
    }
    else
    {
        content.assign(visibleContent);
    }

    if (visibleContent.size() < innerWidth)
    {
        content.append(innerWidth - visibleContent.size(), ' ');
    }

    return "│ " + content + " │";
}

[[nodiscard]] std::string formatCompactBodyLine(const UiSettings& settings,
                                                const std::string& label,
                                                const std::string& value,
                                                const std::size_t innerWidth,
                                                const std::string& valueColor)
{
    const std::string PlainRow = formatCompactRow(label, value);
    if (!settings.colors)
    {
        return formatCompactBoxLine(settings, PlainRow, innerWidth);
    }

    std::string paddedLabel = label;
    if (paddedLabel.size() < CompactLabelColumnWidth)
    {
        paddedLabel.append(CompactLabelColumnWidth - paddedLabel.size(), ' ');
    }

    const std::string& ValueColor = valueColor.empty() ? settings.palette.text : valueColor;
    std::string content = colorizeText(settings, settings.palette.muted, paddedLabel) +
                          colorizeText(settings, ValueColor, value);
    if (PlainRow.size() < innerWidth)
    {
        content.append(innerWidth - PlainRow.size(), ' ');
    }

    return "│ " + content + " │";
}

[[nodiscard]] std::vector<std::string> formatCompactRunEnd(const UiSettings& settings,
                                                           const bool success,
                                                           const double durationSeconds,
                                                           const logging::RunSummary& summary)
{
    const std::string Title = success ? "BUILD SUCCESSFUL" : "BUILD FAILED";

    struct BodyRow
    {
        std::string label;
        std::string value;
        std::string valueColor;
    };

    std::vector<BodyRow> bodyRows;
    bodyRows.push_back({.label = "Time",
                        .value = formatCompactTimeValue(success, durationSeconds),
                        .valueColor = {}});
    if (settings.showTimeSaved && summary.estimatedTimeSavedSeconds > 0.0)
    {
        bodyRows.push_back({.label = "Saved",
                            .value = formatDurationApprox(summary.estimatedTimeSavedSeconds),
                            .valueColor = settings.palette.cacheHit});
    }

    if (!success)
    {
        if (const auto FailedSegment = findFailedSegmentName(summary))
        {
            bodyRows.push_back({.label = "Phase",
                                .value = std::string(*FailedSegment),
                                .valueColor = settings.palette.error});
        }
    }

    bodyRows.push_back({.label = "Cache",
                        .value = formatCacheStats(summary.cacheHitsSkipped, summary.totalSteps),
                        .valueColor = {}});
    bodyRows.push_back({.label = "Peak",
                        .value = std::to_string(displayWorkerCount(summary)) + " workers",
                        .valueColor = settings.palette.info});

    std::vector<std::string> plainRows;
    plainRows.reserve(bodyRows.size());
    std::ranges::transform(bodyRows,
                           std::back_inserter(plainRows),
                           [](const BodyRow& row)
                           { return formatCompactRow(row.label, row.value); });

    const std::size_t InnerWidth =
        std::max({Title.size(), maxContentWidth(plainRows), CompactBoxMinInnerWidth});

    const std::string& TitleColor = success ? settings.palette.success : settings.palette.error;

    std::vector<std::string> lines;
    lines.push_back(formatCompactBoxBorder("╭", "─", "╮", InnerWidth));
    lines.push_back(formatCompactBoxLine(settings, Title, InnerWidth, TitleColor));
    lines.push_back(formatCompactBoxBorder("├", "─", "┤", InnerWidth));

    std::vector<std::string> bodyLines;
    bodyLines.reserve(bodyRows.size());
    std::ranges::transform(bodyRows,
                           std::back_inserter(bodyLines),
                           [&](const BodyRow& row)
                           {
                               return formatCompactBodyLine(
                                   settings, row.label, row.value, InnerWidth, row.valueColor);
                           });
    lines.insert(lines.end(), bodyLines.begin(), bodyLines.end());

    lines.push_back(formatCompactBoxBorder("╰", "─", "╯", InnerWidth));
    return lines;
}

[[nodiscard]] std::string formatSegmentStatus(const UiSettings& settings, const bool success)
{
    if (!success)
    {
        return "[FAIL]";
    }

    if (settings.icons)
    {
        return "[OK]";
    }

    return "OK";
}

[[nodiscard]] std::string formatOverallStatus(const UiSettings& settings, const bool success)
{
    if (!success)
    {
        return settings.icons ? "✗ FAIL" : "FAIL";
    }

    return settings.icons ? "✓ PASS" : "PASS";
}

[[nodiscard]] std::vector<std::string> formatDataRunEnd(const UiSettings& settings,
                                                        const bool success,
                                                        const double durationSeconds,
                                                        const logging::RunSummary& summary)
{
    std::vector<std::string> lines;
    lines.emplace_back("=================================================");

    if (!summary.segments.empty())
    {
        TextTable table({"PHASE", "STATUS", "TIME", "CACHE HIT"});
        for (const auto& segment : summary.segments)
        {
            const std::string CacheHit =
                std::to_string(cacheHitPercent(segment.cacheHits, segment.totalSteps)) + "% (" +
                std::to_string(segment.cacheHits) + '/' + std::to_string(segment.totalSteps) + ')';
            table.addRow({segment.name,
                          formatSegmentStatus(settings, segment.success),
                          formatDurationShort(segment.durationSeconds),
                          CacheHit});
        }

        const std::string Table = table.format();
        std::istringstream stream(Table);
        std::string line;
        while (std::getline(stream, line))
        {
            lines.push_back(std::move(line));
        }
    }
    else
    {
        lines.emplace_back(" PHASE      | STATUS | TIME   | CACHE HIT");
        lines.emplace_back("-------------------------------------------------");
        lines.emplace_back(
            " run        | " + formatSegmentStatus(settings, success) + "   | " +
            formatDurationShort(durationSeconds) + "  | " +
            std::to_string(cacheHitPercent(summary.cacheHitsSkipped, summary.totalSteps)) + "% (" +
            std::to_string(summary.cacheHitsSkipped) + '/' + std::to_string(summary.totalSteps) +
            ')');
    }

    lines.emplace_back("-------------------------------------------------");
    lines.emplace_back(
        " TOTAL      | " + formatOverallStatus(settings, success) + " | " +
        formatDurationShort(durationSeconds) + "  |  " +
        std::to_string(cacheHitPercent(summary.cacheHitsSkipped, summary.totalSteps)) +
        "% overall");
    lines.emplace_back("=================================================");
    return lines;
}

}  // namespace

std::vector<std::string> formatRunEndMessage(const UiSettings& settings,
                                             const bool success,
                                             const double durationSeconds,
                                             const logging::RunSummary& summary)
{
    std::vector<std::string> lines;

    const std::string SavedLine = formatRunSummaryLine(settings, summary);
    if (!SavedLine.empty())
    {
        lines.push_back(SavedLine);
    }

    lines.emplace_back(RunEndSeparator);

    switch (settings.summaryStyle)
    {
    case RunSummaryStyle::Minimal:
        if (success)
        {
            lines.push_back("Build successful in " + formatDurationShort(durationSeconds) + '!');
        }
        else
        {
            lines.push_back("Build failed after " + formatDurationShort(durationSeconds) + '.');
        }
        break;
    case RunSummaryStyle::Simple:
        lines.push_back(formatSimpleRunEnd(settings, success, durationSeconds, summary));
        break;
    case RunSummaryStyle::Compact:
    {
        const auto CompactLines = formatCompactRunEnd(settings, success, durationSeconds, summary);
        lines.insert(lines.end(), CompactLines.begin(), CompactLines.end());
        break;
    }
    case RunSummaryStyle::Data:
    {
        const auto DataLines = formatDataRunEnd(settings, success, durationSeconds, summary);
        lines.insert(lines.end(), DataLines.begin(), DataLines.end());
        break;
    }
    }

    return lines;
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
