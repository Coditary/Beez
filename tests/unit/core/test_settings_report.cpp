#include "beez/cli/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/settings.hpp"
#include "beez/core/settings_report.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(SettingsReportTest, ShowsCliAndProjectOrigins)
{
    const beez::core::Context Context;
    beez::core::BeezSettings globalSettings;
    globalSettings.performance.maxThreads = 4;
    globalSettings.cache.hash.algorithm = "fnv1a32";

    beez::core::BeezSettings projectSettings;
    projectSettings.performance.maxThreads = 8;
    projectSettings.cache.hash.algorithm = "crc32";
    projectSettings.cache.protect = true;

    beez::core::BeezSettings activeSettings = globalSettings;
    activeSettings.merge(projectSettings);

    beez::cli::ParsedOptions cliOptions;
    cliOptions.verbose = true;
    cliOptions.maxThreads = 12;
    activeSettings.applyCliOverrides(cliOptions);

    const std::string Report = beez::core::formatActiveConfiguration(
        beez::core::SettingsReportInput {.globalSettings = globalSettings,
                                         .globalConfigPath = "/home/user/.config/beez/config.lua",
                                         .projectSettings = projectSettings,
                                         .activeSettings = activeSettings,
                                         .cliOptions = cliOptions,
                                         .context = Context});

    EXPECT_NE(Report.find("=== Beez Active Configuration ==="), std::string::npos);
    EXPECT_NE(Report.find("[Performance]"), std::string::npos);
    EXPECT_NE(Report.find("performance.max_threads"), std::string::npos);
    EXPECT_NE(Report.find("(Origin: CLI --threads)"), std::string::npos);
    EXPECT_NE(Report.find("(Origin: CLI --verbose)"), std::string::npos);
    EXPECT_NE(Report.find("(Origin: build.lua)"), std::string::npos);
    EXPECT_NE(Report.find("cache.hash.algorithm"), std::string::npos);
    EXPECT_NE(Report.find("\"crc32\""), std::string::npos);
}

TEST(SettingsReportTest, ShowsDefaultOriginWhenUnset)
{
    const beez::core::Context Context;
    const beez::core::BeezSettings Settings;
    const beez::cli::ParsedOptions CliOptions;

    const std::string Report = beez::core::formatActiveConfiguration(
        beez::core::SettingsReportInput {.globalSettings = Settings,
                                         .globalConfigPath = {},
                                         .projectSettings = Settings,
                                         .activeSettings = Settings,
                                         .cliOptions = CliOptions,
                                         .context = Context});

    EXPECT_NE(Report.find("(Origin: Default)"), std::string::npos);
    EXPECT_NE(Report.find("ui.output_mode"), std::string::npos);
    EXPECT_NE(Report.find("\"clean\""), std::string::npos);
}

TEST(SettingsReportTest, EnvironmentUsesProjectOverrideOrigin)
{
    const beez::core::Context Context;
    beez::core::BeezSettings globalSettings;
    globalSettings.environment["BUILD_TYPE"] = "Debug";

    beez::core::BeezSettings projectSettings;
    projectSettings.environment["BUILD_TYPE"] = "Release";
    projectSettings.environment["CC"] = "clang";

    beez::core::BeezSettings activeSettings = globalSettings;
    activeSettings.merge(projectSettings);

    const std::string Report = beez::core::formatActiveConfiguration(
        beez::core::SettingsReportInput {.globalSettings = globalSettings,
                                         .globalConfigPath = "/home/user/.config/beez/config.lua",
                                         .projectSettings = projectSettings,
                                         .activeSettings = activeSettings,
                                         .cliOptions = {},
                                         .context = Context});

    EXPECT_NE(Report.find("[Environment]"), std::string::npos);
    EXPECT_NE(Report.find("environment.BUILD_TYPE"), std::string::npos);
    EXPECT_NE(Report.find("\"Release\""), std::string::npos);
    EXPECT_NE(Report.find("environment.CC"), std::string::npos);
}
