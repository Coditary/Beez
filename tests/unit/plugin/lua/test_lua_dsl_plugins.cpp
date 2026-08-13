#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <string>

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(LuaDslPluginTest, LoadsPluginStepsViaReqpack)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    description = "Demo plugin",
    steps = {
        compile = {
            phase = "compile",
            scope = "code",
            description = "Compile sources",
            run = "echo compile",
        },
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/demo",
            path = "./plugins/coditary/demo",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("compile");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->phase, "compile");
    EXPECT_EQ(Found->scope, "code");
    ASSERT_TRUE(Found->description.has_value());
    EXPECT_EQ(Found->description.value(), "Compile sources");
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "echo compile");
}

TEST(LuaDslPluginTest, LoadsLazyStepDefinition)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/unnamed/lazy/2.0.0",
                          R"(
plugin("lazy", {
    version = "2.0.0",
    steps = {
        build = function()
            return {
                phase = "build",
                scope = "app",
                run = "echo lazy",
            }
        end,
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "unnamed/lazy",
            path = "./plugins/unnamed/lazy",
            version = "2.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("build");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->phase, "build");
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "echo lazy");
}

TEST(LuaDslPluginTest, PluginSandboxRejectsTaskCalls)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/unnamed/evil/1.0.0",
                          R"(
task("hack", "echo hacked")
plugin("evil", {
    version = "1.0.0",
    steps = {},
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "unnamed/evil",
            path = "./plugins/unnamed/evil",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsMissingPluginPath)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/missing",
            path = "./plugins/coditary/missing",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsCacheOnlyPluginEntry)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    beez = {
        { name = "coditary/demo", version = "1.0.0" },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsUnqualifiedPluginName)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "demo",
            path = "./plugins/coditary/demo",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, StringRequireStillWorks)
{
    const beez::test::TempProject Project;
    std::ofstream(Project.path() / "config.lua") << "return { marker = 'ok' }\n";

    Project.writeBuildLua(R"(
local config = require("config")
task("show", "echo " .. config.marker)
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "show");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    beez::test::expectShellCommand(Found, 0, "echo ok");
}

TEST(LuaDslPluginTest, LoadsPluginFromLocalPath)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("vendor/coditary/local/1.0.0",
                          R"(
plugin("local", {
    version = "1.0.0",
    steps = {
        run = {
            phase = "generate",
            scope = "demo",
            run = "echo local path",
        },
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/local",
            path = "./vendor/coditary/local",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("run");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "echo local path");
}

TEST(LuaDslPluginTest, LoadsPluginFromLocalPathWithoutVersionSubfolder)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("vendor/coditary/direct",
                          R"(
plugin("direct", {
    version = "3.0.0",
    steps = {
        run = {
            phase = "generate",
            scope = "demo",
            run = "echo direct path",
        },
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/direct",
            path = "./vendor/coditary/direct",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("run");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_TRUE(Found->hasShellRun());
    EXPECT_EQ(Found->shellRun.value_or(""), "echo direct path");
}

TEST(LuaDslPluginTest, LoadsPluginStepWithConfigAndLuaRun)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/rich/1.0.0",
                          R"(
plugin("rich", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "yes" },
            run = function(ctx)
                if ctx.config.flag ~= "yes" then
                    return 1
                end
                return 0
            end,
        },
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/rich",
            path = "./plugins/coditary/rich",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_TRUE(Found->hasConfig());
    EXPECT_TRUE(Found->hasCallback());
    EXPECT_FALSE(Found->hasShellRun());
}
