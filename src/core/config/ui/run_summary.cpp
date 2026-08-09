// NOLINTBEGIN(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
#include "beez/core/config/ui_options.hpp"

#include "beez/logging/contract/run_types.hpp"

#include "beez/core/util/text_table.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace beez::core
{
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
