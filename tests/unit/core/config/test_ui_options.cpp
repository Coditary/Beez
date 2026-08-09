#include "beez/core/config/ui_options.hpp"

#include "beez/logging/contract/run_types.hpp"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

std::string joinMessageLines(const std::vector<std::string>& lines)
{
    std::string joined;
    for (const auto& line : lines)
    {
        joined += line;
        joined += '\n';
    }

    return joined;
}

void expectContains(const std::string& haystack, const std::string& needle)
{
    EXPECT_NE(haystack.find(needle), std::string::npos) << needle;
}

void expectMissing(const std::string& haystack, const std::string& needle)
{
    EXPECT_EQ(haystack.find(needle), std::string::npos) << needle;
}

}  // namespace

TEST(UiOptionsTest, ParsesProgressAndIndicatorStyles)
{
    EXPECT_EQ(beez::core::parseProgressDisplayStyle("blocks"),
              beez::core::ProgressDisplayStyle::Blocks);
    EXPECT_EQ(beez::core::parseProgressIndicatorStyle("dots"),
              beez::core::ProgressIndicatorStyle::SpinnerDots);
    EXPECT_EQ(beez::core::parseProgressIndicatorStyle("step"),
              beez::core::ProgressIndicatorStyle::Step);
    EXPECT_EQ(beez::core::parseUiLogLevel("warn"), beez::core::UiLogLevel::Warn);
}

TEST(UiOptionsTest, ResolvesNamedThemeFromThemesTable)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.themes = std::map<std::string, beez::core::UiColorPalette> {
        {"mine",
         beez::core::UiColorPalette {
             .text = "#ffffff",
             .accent = "#ff0000",
         }},
    };
    overlay.theme = "mine";

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_EQ(Settings.palette.text, "#ffffff");
    EXPECT_EQ(Settings.palette.accent, "#ff0000");
}

TEST(UiOptionsTest, UsesDefaultPaletteWhenColorsEnabledAndNoTheme)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.colors = true;

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_EQ(Settings.palette.text, beez::core::defaultColorPalette().text);
    EXPECT_EQ(Settings.palette.success, beez::core::defaultColorPalette().success);
    EXPECT_TRUE(Settings.truecolor);
}

TEST(UiOptionsTest, UsesEmptyPaletteWhenColorsDisabledAndNoTheme)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.colors = false;

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_TRUE(Settings.palette.text.empty());
    EXPECT_TRUE(Settings.palette.accent.empty());
    EXPECT_FALSE(Settings.truecolor);
}

TEST(UiOptionsTest, EnablesTruecolorAutomaticallyWhenColorsEnabled)
{
    const beez::core::UiSettings Settings = beez::core::resolveUiSettings({});
    EXPECT_TRUE(Settings.colors);
    EXPECT_TRUE(Settings.truecolor);
}

TEST(UiOptionsTest, RespectsExplicitTruecolorOverride)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.colors = true;
    overlay.truecolor = false;

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_TRUE(Settings.colors);
    EXPECT_FALSE(Settings.truecolor);
}

TEST(UiOptionsTest, FormatsMinimalProgressLine)
{
    beez::core::UiSettings settings;
    settings.colors = false;

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 2,
                                                                .total = 5,
                                                                .category = "compile",
                                                                .detail = "step: build",
                                                            });

    EXPECT_NE(Line.find("2/5"), std::string::npos);
    EXPECT_NE(Line.find("compile"), std::string::npos);
    EXPECT_NE(Line.find("step: build"), std::string::npos);
}

TEST(UiOptionsTest, FormatsPercentIndicator)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Blocks;
    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::Percent;

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 1,
                                                                .total = 4,
                                                                .category = "test",
                                                                .detail = "step: one",
                                                            });

    EXPECT_NE(Line.find("25%"), std::string::npos);
}

TEST(UiOptionsTest, FormatsSpinnerIndicator)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Minimal;
    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerMinimal;

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 2,
                                                                .total = 4,
                                                                .category = "test",
                                                                .detail = "step: two",
                                                            });

    EXPECT_NE(Line.find('/'), std::string::npos);
    EXPECT_EQ(Line.find("2/4"), std::string::npos);
}

TEST(UiOptionsTest, DefaultsToStepIndicator)
{
    const beez::core::UiSettings Settings = beez::core::resolveUiSettings({});
    EXPECT_EQ(Settings.animation.progress.indicator, beez::core::ProgressIndicatorStyle::Step);
}

TEST(UiOptionsTest, MapsLegacySpinnerConfigToIndicator)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.animation = beez::core::UiAnimationOverlay {
        .spinner = "dots",
    };

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_EQ(Settings.animation.progress.indicator,
              beez::core::ProgressIndicatorStyle::SpinnerDots);
}

TEST(UiOptionsTest, FormatsDotsIndicatorWithoutDelimiters)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Minimal;
    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerDots;
    settings.animation.progress.indicatorStyle.startDelimiter = {};
    settings.animation.progress.indicatorStyle.endDelimiter = {};

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 1,
                                                                .total = 4,
                                                                .category = "test",
                                                                .detail = "step: one",
                                                            });

    EXPECT_EQ(Line.find('['), std::string::npos);
    EXPECT_EQ(Line.find(']'), std::string::npos);
    EXPECT_NE(Line.find("test"), std::string::npos);
}

TEST(UiOptionsTest, AppliesBracketDelimitersForStepIndicatorByDefault)
{
    const beez::core::UiSettings Settings = beez::core::resolveUiSettings({});
    EXPECT_EQ(Settings.animation.progress.indicatorStyle.startDelimiter, "[");
    EXPECT_EQ(Settings.animation.progress.indicatorStyle.endDelimiter, "]");
}

TEST(UiOptionsTest, AppliesEmptyDelimitersForDotsIndicatorByDefault)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.animation = beez::core::UiAnimationOverlay {
        .indicator = "dots",
    };

    const beez::core::UiSettings Settings = beez::core::resolveUiSettings(overlay);
    EXPECT_TRUE(Settings.animation.progress.indicatorStyle.startDelimiter.empty());
    EXPECT_TRUE(Settings.animation.progress.indicatorStyle.endDelimiter.empty());
}

TEST(UiOptionsTest, EnablesAnimatedSpinnerForSpinnerIndicators)
{
    beez::core::UiSettings settings;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Minimal;
    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerDots;
    EXPECT_TRUE(beez::core::usesAnimatedProgressSpinner(settings));

    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Blocks;
    EXPECT_TRUE(beez::core::usesAnimatedProgressSpinner(settings));

    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::Step;
    EXPECT_FALSE(beez::core::usesAnimatedProgressSpinner(settings));

    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerDots;
    settings.animation.progress.indicatorSpinIntervalMs = 0;
    EXPECT_FALSE(beez::core::usesAnimatedProgressSpinner(settings));
}

TEST(UiOptionsTest, FormatsProgressLineWithExplicitSpinnerFrame)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Blocks;
    settings.animation.progress.indicator = beez::core::ProgressIndicatorStyle::SpinnerMinimal;

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 1,
                                                                .total = 2,
                                                                .category = "test",
                                                                .detail = "step",
                                                            },
                                                            false,
                                                            2U);

    EXPECT_NE(Line.find('-'), std::string::npos);
}

TEST(UiOptionsTest, FormatsBlocksProgressLine)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.animation.progress.style = beez::core::ProgressDisplayStyle::Blocks;

    const std::string Line = beez::core::formatProgressLine(settings,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 1,
                                                                .total = 2,
                                                                .category = "test",
                                                                .detail = "step: one",
                                                            });

    EXPECT_NE(Line.find("█"), std::string::npos);
    EXPECT_NE(Line.find("░"), std::string::npos);
}

TEST(UiOptionsTest, UsesWorkerNameForPrefixLabel)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.workerPrefixEnabled = true;
    settings.workerPrefixFormat = "Worker {id}";

    const std::string Prefix = beez::core::formatWorkerOutputPrefix(settings, 99, "tidy_3");
    EXPECT_NE(Prefix.find("Worker tidy_3"), std::string::npos);
    EXPECT_EQ(Prefix.find("Worker 99"), std::string::npos);
}

TEST(UiOptionsTest, UsesPlainWorkerPrefixByDefault)
{
    const beez::core::UiSettings Settings = beez::core::resolveUiSettings({});
    EXPECT_FALSE(Settings.workerPrefixEnabled);
    EXPECT_EQ(beez::core::formatWorkerOutputPrefix(Settings, 4, "fmt_cpp_1"), "  | ");
}

TEST(UiOptionsTest, FormatsWorkerPrefix)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.workerPrefixEnabled = true;
    settings.workerPrefixFormat = "[Worker {id}]";

    const std::string Prefix = beez::core::formatWorkerOutputPrefix(settings, 4, {});
    EXPECT_NE(Prefix.find("[Worker 4]"), std::string::npos);
}

TEST(UiOptionsTest, FormatsRunSummaryWhenEnabled)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.summaryStyle = beez::core::RunSummaryStyle::Minimal;
    settings.showTimeSaved = true;
    settings.palette.cacheHit = "#8ec07c";

    const std::string Line = beez::core::formatRunSummaryLine(
        settings, beez::logging::RunSummary {.cacheHitsSkipped = 3});
    EXPECT_NE(Line.find('3'), std::string::npos);
    EXPECT_NE(Line.find("cache hits"), std::string::npos);
}

TEST(UiOptionsTest, FormatsSimpleRunEndSummary)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.icons = false;
    settings.summaryStyle = beez::core::RunSummaryStyle::Simple;
    settings.showTimeSaved = true;

    const auto Lines = beez::core::formatRunEndMessage(
        settings,
        true,
        0.12,
        beez::logging::RunSummary {.cacheHitsSkipped = 142,
                                   .totalSteps = 150,
                                   .workerThreads = 8,
                                   .estimatedTimeSavedSeconds = 134.0});

    ASSERT_FALSE(Lines.empty());
    const std::string& summaryLine = Lines.back();
    EXPECT_NE(summaryLine.find("Build finished in 0.12s"), std::string::npos);
    EXPECT_NE(summaryLine.find("142/150 cached (94%)"), std::string::npos);
    EXPECT_NE(summaryLine.find("8 workers"), std::string::npos);
    EXPECT_NE(summaryLine.find("saved ~2m 14s"), std::string::npos);
}

TEST(UiOptionsTest, FormatsFullyCachedRunEndWithSavedTime)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.icons = false;
    settings.summaryStyle = beez::core::RunSummaryStyle::Simple;
    settings.showTimeSaved = true;

    const auto Lines = beez::core::formatRunEndMessage(
        settings,
        true,
        0.01,
        beez::logging::RunSummary {.cacheHitsSkipped = 188,
                                   .totalSteps = 188,
                                   .workerThreads = 16,
                                   .estimatedTimeSavedSeconds = 0.47});

    ASSERT_FALSE(Lines.empty());
    const std::string& summaryLine = Lines.back();
    EXPECT_NE(summaryLine.find("188/188 cached (100%)"), std::string::npos);
    EXPECT_NE(summaryLine.find("saved ~0.47s"), std::string::npos);
}

TEST(UiOptionsTest, FormatsFailedSimpleRunEndSummary)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.icons = false;
    settings.summaryStyle = beez::core::RunSummaryStyle::Simple;

    const auto Lines = beez::core::formatRunEndMessage(
        settings, false, 0.12, beez::logging::RunSummary {.workerThreads = 4});

    ASSERT_FALSE(Lines.empty());
    EXPECT_NE(Lines.back().find("Build failed after 0.12s"), std::string::npos);
}

TEST(UiOptionsTest, FormatsCompactRunEndSummary)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.icons = true;
    settings.summaryStyle = beez::core::RunSummaryStyle::Compact;
    settings.showTimeSaved = true;

    const std::string Output = joinMessageLines(beez::core::formatRunEndMessage(
        settings,
        true,
        0.13,
        beez::logging::RunSummary {.cacheHitsSkipped = 2,
                                   .totalSteps = 6,
                                   .peakWorkers = 8,
                                   .estimatedTimeSavedSeconds = 1.18}));

    expectContains(Output, "BUILD SUCCESSFUL");
    expectMissing(Output, "BEEZ");
    expectContains(Output, "Time    finished in 0.13s");
    expectContains(Output, "Saved   ~1.18s");
    expectContains(Output, "Cache   2/6 cached (33%)");
    expectContains(Output, "Peak    8 workers");
    expectMissing(Output, "Saved approx");
    expectMissing(Output, "active threads");
}

TEST(UiOptionsTest, FormatsCompactFailedRunEndSummary)
{
    beez::core::UiSettings settings;
    settings.colors = false;
    settings.icons = true;
    settings.summaryStyle = beez::core::RunSummaryStyle::Compact;
    settings.showTimeSaved = true;

    const std::string Output = joinMessageLines(beez::core::formatRunEndMessage(
        settings,
        false,
        0.06,
        beez::logging::RunSummary {.cacheHitsSkipped = 0,
                                   .totalSteps = 1,
                                   .peakWorkers = 2,
                                   .segments = {{.name = "qa:code",
                                                 .success = false,
                                                 .durationSeconds = 0.06,
                                                 .cacheHits = 0,
                                                 .totalSteps = 1}}}));

    expectContains(Output, "BUILD FAILED");
    expectMissing(Output, "BEEZ");
    expectContains(Output, "Time    failed after 0.06s");
    expectContains(Output, "Phase   qa:code");
    expectContains(Output, "Cache   0/1 cached (0%)");
    expectContains(Output, "Peak    2 workers");
}

TEST(UiOptionsTest, RejectsUnknownThemeName)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.theme = "missing";

    EXPECT_THROW(static_cast<void>(beez::core::resolveUiSettings(overlay)), std::runtime_error);
}
