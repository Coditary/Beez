#include "beez/core/ui_options.hpp"
#include "beez/logging/logger.hpp"
#include "beez/plugin/lua/lua_settings.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/core/settings.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

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
