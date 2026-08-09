#include "beez/cli/parsing/parsed_options.hpp"
#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/env_settings.hpp"
#include "beez/core/settings.hpp"
#include "beez/logging/output_mode.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

TEST(BeezSettingsTest, MergeOrderGlobalProjectThenCli)
{
    beez::core::BeezSettings settings;
    settings.performance.maxThreads = 2;
    settings.ui.outputMode = beez::logging::OutputMode::Clean;
    settings.cache.enabled = true;

    beez::core::BeezSettings project;
    project.performance.maxThreads = 4;
    project.ui.outputMode = beez::logging::OutputMode::Verbose;
    settings.merge(project);

    beez::cli::ParsedOptions cli;
    cli.maxThreads = 8;
    cli.verbose = true;
    cli.enableCache = false;
    settings.applyCliOverrides(cli);

    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 8U);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Verbose);
    ASSERT_TRUE(settings.cache.enabled.has_value());
    EXPECT_FALSE(*settings.cache.enabled);
}

TEST(BeezSettingsTest, MergePrefersOverlayValues)
{
    beez::core::BeezSettings base;
    base.performance.maxThreads = 4;
    base.env.vars["FOO"] = "global";

    beez::core::BeezSettings overlay;
    overlay.performance.maxThreads = 8;
    overlay.env.vars["FOO"] = "project";
    overlay.env.vars["BAR"] = "added";

    base.merge(overlay);

    ASSERT_TRUE(base.performance.maxThreads.has_value());
    EXPECT_EQ(*base.performance.maxThreads, 8U);
    EXPECT_EQ(base.env.vars.at("FOO"), "project");
    EXPECT_EQ(base.env.vars.at("BAR"), "added");
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
    settings.cache.compress.mode = "always";

    const auto Options = settings.resolveCacheOptions(Context);

    EXPECT_TRUE(Options.enabled);
    EXPECT_TRUE(Options.protect);
    EXPECT_EQ(Options.hash.algorithm, beez::core::ContentHashAlgorithm::Crc32);
    EXPECT_EQ(Options.hash.seed, 9U);
    EXPECT_EQ(Options.compress.algorithm, beez::core::CacheCompressionAlgorithm::Gzip);
    EXPECT_EQ(Options.compress.level, 4);
    EXPECT_EQ(Options.compress.mode, beez::core::CacheCompressionMode::Always);
}

TEST(BeezSettingsTest, CacheEnabledUsesCacheSectionOnly)
{
    const beez::core::Context Context;
    beez::core::BeezSettings settings;
    settings.cache.enabled = false;

    EXPECT_FALSE(settings.resolveCacheOptions(Context).enabled);
}

TEST(BeezSettingsTest, CliSilentAndErrorOverrideOutputMode)
{
    beez::core::BeezSettings settings;
    settings.ui.outputMode = beez::logging::OutputMode::Verbose;

    beez::cli::ParsedOptions silentOptions;
    silentOptions.silent = true;
    settings.applyCliOverrides(silentOptions);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Silent);

    beez::cli::ParsedOptions errorOptions;
    errorOptions.errorsOnly = true;
    settings.applyCliOverrides(errorOptions);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Errors);

    beez::cli::ParsedOptions verboseOptions;
    verboseOptions.verbose = true;
    settings.applyCliOverrides(verboseOptions);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Verbose);
}

TEST(BeezSettingsTest, CliSilentTakesPriorityOverVerbose)
{
    beez::core::BeezSettings settings;
    beez::cli::ParsedOptions options;
    options.silent = true;
    options.verbose = true;
    settings.applyCliOverrides(options);

    ASSERT_TRUE(settings.ui.outputMode.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Silent);
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

TEST(BeezSettingsTest, CliDryRunSetsDryRunFlag)
{
    beez::core::BeezSettings settings;
    beez::cli::ParsedOptions options;
    options.dryRun = true;

    settings.applyCliOverrides(options);

    ASSERT_TRUE(settings.dryRun.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate
    EXPECT_EQ(settings.dryRun.value(), true);
    EXPECT_TRUE(settings.toRunOptions(nullptr, beez::core::Context()).dryRun);
}

TEST(BeezSettingsTest, ApplyEnvironmentSetsVars)
{
    const beez::core::Context Context;
    beez::core::BeezSettings settings;
    settings.env.vars["BEEZ_TEST_ENV_VAR"] = "configured";

    settings.applyEnvironment(Context);

    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
    const char* value = std::getenv("BEEZ_TEST_ENV_VAR");
    ASSERT_NE(value, nullptr);
    EXPECT_STREQ(value, "configured");
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

TEST(EnvSettingsTest, ResolveUsesDefaultsForHashLists)
{
    const beez::core::EnvSettings Resolved = beez::core::resolveEnvSettings({});
    EXPECT_TRUE(Resolved.loadDotenv);
    EXPECT_FALSE(Resolved.dotenvOverridesSystem);
    EXPECT_FALSE(Resolved.hashVars.empty());
    EXPECT_FALSE(Resolved.ignoreVarsForHashing.empty());
    EXPECT_FALSE(Resolved.maskSecrets.empty());
}
