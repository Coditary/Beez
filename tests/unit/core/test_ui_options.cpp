#include "beez/core/ui_options.hpp"

#include "beez/logging/logger.hpp"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>

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

TEST(UiOptionsTest, UsesEmptyPaletteWhenNoThemeSelected)
{
    const beez::core::UiSettings Settings = beez::core::resolveUiSettings({});
    EXPECT_TRUE(Settings.palette.text.empty());
    EXPECT_TRUE(Settings.palette.accent.empty());
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
    settings.showTimeSaved = true;
    settings.palette.cacheHit = "#8ec07c";

    const std::string Line = beez::core::formatRunSummaryLine(
        settings, beez::logging::RunSummary {.cacheHitsSkipped = 3});
    EXPECT_NE(Line.find('3'), std::string::npos);
    EXPECT_NE(Line.find("cached steps"), std::string::npos);
}

TEST(UiOptionsTest, RejectsUnknownThemeName)
{
    beez::core::UiSettingsOverlay overlay;
    overlay.theme = "missing";

    EXPECT_THROW(static_cast<void>(beez::core::resolveUiSettings(overlay)), std::runtime_error);
}
