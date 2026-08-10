// NOLINTBEGIN(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
#include "beez/core/config/ui/progress_format.hpp"

#include "beez/core/config/ui/theme.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace beez::core
{

namespace
{

constexpr std::array<const char*, 4> MinimalSpinnerFrames = {"|", "/", "-", "\\"};
constexpr std::array<const char*, 8> DotsSpinnerFrames = {"⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"};

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
        const std::string emptyGlyph = defaultEmptyGlyph(useBlocks, style.emptyChar);
        std::string bar;
        for (std::size_t character = 0; character < width; ++character)
        {
            bar += emptyGlyph;
        }
        return bar;
    }

    const std::size_t filled = std::min(width, (index * width + total - 1) / total);
    const std::string fillGlyph = defaultFillGlyph(useBlocks, style.fillChar);
    const std::string emptyGlyph = defaultEmptyGlyph(useBlocks, style.emptyChar);

    std::string bar;
    for (std::size_t character = 0; character < filled; ++character)
    {
        bar += fillGlyph;
    }
    for (std::size_t character = filled; character < width; ++character)
    {
        bar += emptyGlyph;
    }

    return bar;
}

}  // namespace

std::string formatProgressLine(const UiSettings& settings,
                               const logging::ExecutionProgress& progress,
                               const bool cached,
                               const std::optional<std::size_t> spinnerFrame)
{
    const auto indicator = formatProgressIndicator(
        settings.animation.progress, progress.index, progress.total, spinnerFrame);
    const std::string indicatorSegment = formatIndicatorSegment(settings, indicator);
    const std::string category = cached ? progress.category + " (cached)" : progress.category;
    const std::string& accent = cached ? settings.palette.cacheHit : settings.palette.accent;
    const std::string coloredCategory = colorizeText(settings, accent, category);
    const std::string coloredDetail =
        colorizeText(settings, settings.palette.text, progress.detail);

    switch (settings.animation.progress.style)
    {
    case ProgressDisplayStyle::Minimal:
        return indicatorSegment + ' ' + coloredCategory + " | " + coloredDetail;
    case ProgressDisplayStyle::Lines:
    case ProgressDisplayStyle::Blocks:
    case ProgressDisplayStyle::Custom:
    {
        const bool useBlocks = settings.animation.progress.style == ProgressDisplayStyle::Blocks;
        const auto& custom = settings.animation.progress.custom;
        const std::string bar = renderBar(custom, progress.index, progress.total, 20, useBlocks);
        const std::string coloredBar = colorizeText(settings, settings.palette.progressFill, bar);
        return custom.startDelimiter + coloredBar + custom.endDelimiter + ' ' + indicatorSegment +
               ' ' + coloredCategory + " | " + coloredDetail;
    }
    }

    return coloredCategory + " | " + coloredDetail;
}

std::string formatWorkerOutputPrefix(const UiSettings& settings,
                                     const std::uint64_t channelId,
                                     const std::string_view channelLabel)
{
    if (!settings.workerPrefixEnabled)
    {
        return "  | ";
    }

    const std::string identifier =
        channelLabel.empty() ? std::to_string(channelId) : std::string(channelLabel);
    const std::string prefix = replaceToken(settings.workerPrefixFormat, "{id}", identifier);
    return colorizeText(settings, settings.palette.workerPrefix, prefix) + ' ';
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
