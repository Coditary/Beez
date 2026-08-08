#include "beez/plugin/lua/lua_settings.hpp"

#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/core/settings.hpp"
#include "beez/logging/output_mode.hpp"
#include "beez/plugin/lua/lua_dsl.h"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

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
    environment = {
        BEEZ_TEST_CONFIG = "loaded",
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
    EXPECT_EQ(settings.environment.at("BEEZ_TEST_CONFIG"), "loaded");
}

TEST(LuaSettingsTest, BuildLuaConfigOverridesGlobalSettings)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    performance = {
        max_threads = 3,
    },
    environment = {
        BEEZ_TEST_CONFIG = "build",
    },
})
task("noop", "true")
)");

    beez::core::BeezSettings settings;
    settings.performance.maxThreads = 12;
    settings.environment["BEEZ_TEST_CONFIG"] = "global";

    const beez::core::Context Context(Project.path());
    beez::core::Registry registry;
    beez::plugin::lua::LuaDslLoader loader;
    ASSERT_TRUE(loader.load(Context, registry));

    settings.merge(loader.buildSettings());

    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- gtest ASSERT_TRUE does not propagate
    ASSERT_TRUE(settings.performance.maxThreads.has_value());
    EXPECT_EQ(*settings.performance.maxThreads, 3U);
    // NOLINTEND(bugprone-unchecked-optional-access)
    EXPECT_EQ(settings.environment.at("BEEZ_TEST_CONFIG"), "build");
}
