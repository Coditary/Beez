#include "beez/core/config/ui/progress_format.hpp"
#include "beez/core/config/ui/types.hpp"
#include "beez/logging/contract/run_types.hpp"
#include "beez/plugin/lua/settings/lua_settings.hpp"

#include "beez/core/config/cache/cache_options.hpp"
#include "beez/core/config/settings/settings.hpp"
#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/logging/console/output_mode.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

TEST(LuaSettingsTest, LoadsCacheSettingsFromFile)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    cache = {
        path = "custom-cache",
        enabled = true,
        protect = true,
        hash = {
            algorithm = "crc32",
            seed = 3,
        },
        compress = {
            algorithm = "gzip",
            level = 5,
        },
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));

    const beez::core::Context Context(Project.path());
    const auto Options = settings.resolveCacheOptions(Context);

    EXPECT_EQ(Options.root, Project.path() / "custom-cache");
    EXPECT_TRUE(Options.enabled);
    EXPECT_TRUE(Options.protect);
    EXPECT_EQ(Options.hash.algorithm, beez::core::ContentHashAlgorithm::Crc32);
    EXPECT_EQ(Options.hash.seed, 3U);
    EXPECT_EQ(Options.compress.algorithm, beez::core::CacheCompressionAlgorithm::Gzip);
    EXPECT_EQ(Options.compress.level, 5);
}

TEST(LuaSettingsTest, LoadsSettingsTableFromFile)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    performance = {
        max_threads = 6,
    },
    ui = {
        output_mode = "verbose",
    },
    env = {
        vars = {
            BEEZ_TEST_CONFIG = "loaded",
        },
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));

    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate
    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 6U);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Verbose);
    // NOLINTEND(bugprone-unchecked-optional-access)
    EXPECT_EQ(settings.env.vars.at("BEEZ_TEST_CONFIG"), "loaded");
}

TEST(LuaSettingsTest, BuildLuaConfigOverridesGlobalSettings)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    performance = {
        max_threads = 3,
    },
    env = {
        vars = {
            BEEZ_TEST_CONFIG = "build",
        },
    },
})
task("noop", "true")
)");

    beez::core::BeezSettings settings;
    settings.performance.maxThreads = 12;
    settings.env.vars["BEEZ_TEST_CONFIG"] = "global";

    const beez::core::Context Context(Project.path());
    beez::core::Registry registry;
    beez::plugin::lua::LuaDslLoader loader;
    ASSERT_TRUE(loader.load(Context, registry));

    settings.merge(loader.buildSettings());

    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate
    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 3U);
    // NOLINTEND(bugprone-unchecked-optional-access)
    EXPECT_EQ(settings.env.vars.at("BEEZ_TEST_CONFIG"), "build");
}

TEST(LuaSettingsTest, LoadsBlocksProgressStyleFromConfig)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    ui = {
        animation = {
            progress = "blocks",
        },
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));

    const beez::core::UiSettings Resolved = settings.resolveUiSettings();
    EXPECT_EQ(Resolved.animation.progress.style, beez::core::ProgressDisplayStyle::Blocks);

    const std::string Line = beez::core::formatProgressLine(Resolved,
                                                            beez::logging::ExecutionProgress {
                                                                .index = 1,
                                                                .total = 2,
                                                                .category = "qa",
                                                                .detail = "step",
                                                            });
    EXPECT_NE(Line.find("█"), std::string::npos);
}

TEST(LuaSettingsTest, RejectsInvalidOutputMode)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    ui = {
        output_mode = "loud",
    },
}
)";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsSettingsFileThatDoesNotReturnTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << "return \"not-a-table\"\n";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNegativeMaxThreads)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    performance = {
        max_threads = -1,
    },
}
)";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonStringCachePath)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    cache = {
        path = 42,
    },
}
)";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonTableEnvVars)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    env = {
        vars = "not-a-table",
    },
}
)";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, ReturnsTrueWhenConfigFileMissing)
{
    const beez::test::TempProject Project;
    beez::core::BeezSettings settings;
    EXPECT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(Project.path() / "missing-config.lua",
                                                           settings));
}

TEST(LuaSettingsTest, LoadsComprehensiveConfigSections)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    performance = {
        max_threads = 4,
        cache_write_strategy = "phase",
        cache_fs_metadata = true,
        use_mmap_for_hashing = true,
        mmap_hashing_min_bytes = 1024,
        optimize_gc_for_throughput = true,
        pin_threads_to_cores = false,
    },
    cache = {
        path = "my-cache",
        enabled = false,
        protect = false,
        hash = { algorithm = "djb2", seed = 2 },
        compress = { algorithm = "gzip", level = 3, mode = "auto" },
    },
    ui = {
        output_mode = "errors",
        colors = false,
        truecolor = false,
        icons = false,
        log_level = "warn",
        hide_cache_hits = true,
        prefix = true,
        prefix_format = "[{id}]",
        show_time_saved = false,
        summary = "compact",
        logging = {
            run_log = false,
            run_log_file = "custom-latest.log",
            log_steps = true,
            workers = "always",
            workers_dir = "custom-workers",
        },
        themes = {
            dark = {
                text = "#111111",
                muted = "#222222",
                success = "#333333",
                warning = "#444444",
                error = "#555555",
                info = "#666666",
                accent = "#777777",
                progress_fill = "#888888",
                progress_empty = "#999999",
                cache_hit = "#aaaaaa",
                worker_prefix = "#bbbbbb",
            },
        },
        theme = "dark",
        animation = {
            progress = {
                start = "[",
                end_delimiter = "]",
                fill = "#",
                empty = ".",
                numbers = "percent",
                indicator = {
                    type = "dots",
                    frames = { "a", "b", "c" },
                    start = "<",
                    end_delimiter = ">",
                    spin_interval = 25,
                },
            },
            indicator_spin_interval = 30,
            spinner = "minimal",
        },
    },
    env = {
        load_dotenv = true,
        dotenv_overrides_system = false,
        files = { ".env", ".env.local" },
        vars = { MY_VAR = "value" },
        hash_vars = { "PATH", "HOME" },
        ignore_vars_for_hashing = { "TMPDIR" },
        mask_secrets = { "SECRET" },
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));

    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- ASSERT_TRUE does not propagate
    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 4U);
    ASSERT_TRUE(settings.performance.cacheWriteStrategy.has_value());
    EXPECT_EQ(*settings.performance.cacheWriteStrategy, "phase");
    ASSERT_TRUE(settings.performance.useMmapForHashing.has_value());
    EXPECT_TRUE(*settings.performance.useMmapForHashing);
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Errors);
    ASSERT_TRUE(settings.ui.options.logLevel.has_value());
    EXPECT_EQ(*settings.ui.options.logLevel, "warn");
    ASSERT_TRUE(settings.ui.options.logging.has_value());
    EXPECT_FALSE(*settings.ui.options.logging->runLog);
    EXPECT_TRUE(*settings.ui.options.logging->logSteps);
    ASSERT_TRUE(settings.ui.options.logging->workers.has_value());
    EXPECT_EQ(*settings.ui.options.logging->workers, "always");
    ASSERT_TRUE(settings.ui.options.theme.has_value());
    EXPECT_EQ(*settings.ui.options.theme, "dark");
    ASSERT_TRUE(settings.ui.options.themes.has_value());
    EXPECT_EQ(settings.ui.options.themes->at("dark").text, "#111111");
    ASSERT_TRUE(settings.ui.options.animation.has_value());
    EXPECT_EQ(*settings.ui.options.animation->indicator, "dots");
    // NOLINTEND(bugprone-unchecked-optional-access)

    EXPECT_EQ(settings.env.vars.at("MY_VAR"), "value");
    EXPECT_EQ(settings.env.files.size(), 2U);
    EXPECT_EQ(settings.env.hashVars.size(), 2U);
    EXPECT_EQ(settings.env.ignoreVarsForHashing.size(), 1U);
    EXPECT_EQ(settings.env.maskSecrets.size(), 1U);

    const beez::core::Context Context(Project.path());
    const auto CacheOptions = settings.resolveCacheOptions(Context);
    EXPECT_EQ(CacheOptions.root, Project.path() / "my-cache");
    EXPECT_FALSE(CacheOptions.enabled);
    EXPECT_EQ(CacheOptions.hash.algorithm, beez::core::ContentHashAlgorithm::Djb2);

    const beez::core::UiSettings ResolvedUi = settings.resolveUiSettings();
    EXPECT_FALSE(ResolvedUi.colors);
    EXPECT_EQ(ResolvedUi.summaryStyle, beez::core::RunSummaryStyle::Compact);
    EXPECT_EQ(ResolvedUi.animation.progress.indicatorSpinIntervalMs, 30U);
}

TEST(LuaSettingsTest, LoadsCleanAndSilentOutputModes)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    ui = {
        output_mode = "clean",
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Clean);
    // NOLINTEND(bugprone-unchecked-optional-access)

    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    ui = {
        output_mode = "silent",
    },
}
)";
    }

    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(settings.ui.outputMode.has_value());
    EXPECT_EQ(*settings.ui.outputMode, beez::logging::OutputMode::Silent);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(LuaSettingsTest, LoadsIndicatorFrameListTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(
return {
    ui = {
        animation = {
            indicator = { "1", "2", "3" },
        },
    },
}
)";
    }

    beez::core::BeezSettings settings;
    ASSERT_TRUE(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings));
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(settings.ui.options.animation.has_value());
    ASSERT_TRUE(settings.ui.options.animation->customIndicatorFrames.has_value());
    EXPECT_EQ(settings.ui.options.animation->customIndicatorFrames->size(), 3U);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(LuaSettingsTest, ThrowsWhenLuaScriptFails)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << "return { broken = \n";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonTablePerformanceSection)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { performance = "bad" })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonTableCacheSection)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { cache = 1 })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonTableUiSection)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { ui = false })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsNonTableEnvSection)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { env = 0 })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidUiLoggingTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { ui = { logging = "bad" } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidUiThemesTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { ui = { themes = "bad" } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidUiAnimationTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { ui = { animation = 42 } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidCacheHashTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { cache = { hash = "bad" } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidCacheCompressTable)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { cache = { compress = false } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidEnvVarsEntryTypes)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { env = { vars = { bad = 1 } } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}

TEST(LuaSettingsTest, RejectsInvalidEnvFilesValue)
{
    const beez::test::TempProject Project;
    const auto ConfigPath = Project.path() / "config.lua";
    {
        std::ofstream stream(ConfigPath);
        stream << R"(return { env = { files = 99 } })";
    }

    beez::core::BeezSettings settings;
    EXPECT_THROW(beez::plugin::lua::loadSettingsFromLuaFile(ConfigPath, settings),
                 std::runtime_error);
}
