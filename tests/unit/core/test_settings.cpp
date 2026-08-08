#include "beez/cli/parsed_options.hpp"
#include "beez/core/context.h"
#include "beez/core/settings.hpp"
#include "beez/logging/output_mode.hpp"

#include <gtest/gtest.h>

#include <filesystem>

TEST(BeezSettingsTest, MergePrefersOverlayValues)
{
    beez::core::BeezSettings base;
    base.performance.maxThreads = 4;
    base.environment["FOO"] = "global";

    beez::core::BeezSettings overlay;
    overlay.performance.maxThreads = 8;
    overlay.environment["FOO"] = "project";
    overlay.environment["BAR"] = "added";

    base.merge(overlay);

    ASSERT_TRUE(base.performance.maxThreads.has_value());
    EXPECT_EQ(*base.performance.maxThreads, 8U);
    EXPECT_EQ(base.environment.at("FOO"), "project");
    EXPECT_EQ(base.environment.at("BAR"), "added");
}

TEST(BeezSettingsTest, ResolveCacheDirectorySupportsRelativeAndAbsolutePaths)
{
    const beez::core::Context Context(std::filesystem::temp_directory_path() /
                                      "beez-settings-test");

    beez::core::BeezSettings settings;
    settings.cache.directory = "custom-cache";
    EXPECT_EQ(settings.resolveCacheDirectory(Context), Context.projectRoot() / "custom-cache");

    settings.cache.directory = std::filesystem::path("/tmp/beez-cache");
    EXPECT_EQ(settings.resolveCacheDirectory(Context), std::filesystem::path("/tmp/beez-cache"));
}

TEST(BeezSettingsTest, CliOverridesProjectSettings)
{
    beez::core::BeezSettings settings;
    settings.performance.maxThreads = 4;
    settings.ui.outputMode = beez::logging::OutputMode::Clean;

    beez::cli::ParsedOptions options;
    options.verbose = true;
    options.maxThreads = 16;

    settings.applyCliOverrides(options);

    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 16U);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Verbose);
}

TEST(BeezSettingsTest, ApplyToContextUpdatesPaths)
{
    beez::core::Context context;
    beez::core::BeezSettings settings;
    settings.paths.buildScript = "custom-build.lua";
    settings.paths.envFile = "config/.env";

    settings.applyToContext(context);

    EXPECT_EQ(context.buildScriptPath().filename(), "custom-build.lua");
    EXPECT_EQ(context.envFilePath(), context.projectRoot() / "config/.env");
}
