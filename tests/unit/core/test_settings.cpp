#include "beez/cli/parsed_options.hpp"
#include "beez/core/cache_options.hpp"
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

TEST(BeezSettingsTest, ResolveCacheOptionsSupportsRelativeAndAbsolutePaths)
{
    const beez::core::Context Context(std::filesystem::temp_directory_path() /
                                      "beez-settings-test");

    beez::core::BeezSettings settings;
    settings.cache.path = "custom-cache";
    EXPECT_EQ(settings.resolveCacheOptions(Context).root, Context.projectRoot() / "custom-cache");

    settings.cache.path = std::filesystem::path("/tmp/beez-cache");
    EXPECT_EQ(settings.resolveCacheOptions(Context).root, std::filesystem::path("/tmp/beez-cache"));
}

TEST(BeezSettingsTest, ResolveCacheOptionsAppliesHashCompressAndProtect)
{
    const beez::core::Context Context;
    beez::core::BeezSettings settings;
    settings.cache.enabled = true;
    settings.cache.protect = true;
    settings.cache.hash.algorithm = "crc32";
    settings.cache.hash.seed = 9U;
    settings.cache.compress.algorithm = "gzip";
    settings.cache.compress.level = 4;

    const auto Options = settings.resolveCacheOptions(Context);

    EXPECT_TRUE(Options.enabled);
    EXPECT_TRUE(Options.protect);
    EXPECT_EQ(Options.hash.algorithm, beez::core::ContentHashAlgorithm::Crc32);
    EXPECT_EQ(Options.hash.seed, 9U);
    EXPECT_EQ(Options.compress.algorithm, beez::core::CacheCompressionAlgorithm::Gzip);
    EXPECT_EQ(Options.compress.level, 4);
}

TEST(BeezSettingsTest, CacheEnabledFallsBackToEngineEnableCache)
{
    const beez::core::Context Context;
    beez::core::BeezSettings settings;
    settings.engine.enableCache = false;

    EXPECT_FALSE(settings.resolveCacheOptions(Context).enabled);
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

TEST(BeezSettingsTest, CliNoCacheDisablesCacheSection)
{
    beez::core::BeezSettings settings;
    settings.cache.enabled = true;

    beez::cli::ParsedOptions options;
    options.enableCache = false;
    settings.applyCliOverrides(options);

    ASSERT_TRUE(settings.cache.enabled.has_value());
    EXPECT_FALSE(*settings.cache.enabled);
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

TEST(BeezSettingsTest, ToRunOptionsPassesResolvedCache)
{
    const beez::core::Context Context;
    beez::core::BeezSettings settings;
    settings.cache.path = "runtime-cache";
    settings.cache.enabled = true;
    settings.cache.protect = true;

    const auto Options = settings.toRunOptions(nullptr, Context);
    EXPECT_TRUE(Options.enableCache);
    EXPECT_EQ(Options.cache.root, Context.projectRoot() / "runtime-cache");
    EXPECT_TRUE(Options.cache.protect);
}
