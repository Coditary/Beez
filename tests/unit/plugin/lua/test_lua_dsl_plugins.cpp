#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace
{

// NOLINTBEGIN(misc-include-cleaner)
class ScopedEnv
{
  public:
    ScopedEnv(const char* name, const char* value)
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
        : name_(name), hadValue_(std::getenv(name) != nullptr)
    {
        if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
            saved_ = std::getenv(name);
        }

        if (value[0] == '\0')
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            unsetenv(name);
            unset_ = true;
        }
        else
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            setenv(name, value, 1);
            unset_ = false;
        }
    }

    ~ScopedEnv()
    {
        if (unset_)
        {
            if (hadValue_)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
                setenv(name_.c_str(), saved_.c_str(), 1);
            }
            else
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
                unsetenv(name_.c_str());
            }
        }
        else if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            setenv(name_.c_str(), saved_.c_str(), 1);
        }
    }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

  private:
    std::string name_;
    bool hadValue_ = false;
    bool unset_ = false;
    std::string saved_;
};
// NOLINTEND(misc-include-cleaner)

class PluginHomeFixture
{
  public:
    explicit PluginHomeFixture(const beez::test::TempProject& project)
        : home_(project.path().string()), homeEnv_("HOME", home_.c_str()),
          xdgUnset_("XDG_CACHE_HOME", "")
    {
    }

  private:
    std::string home_;
    ScopedEnv homeEnv_;
    ScopedEnv xdgUnset_;
};

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(LuaDslPluginTest, LoadsPluginStepsViaRequire)
{
    const beez::test::TempProject Project;
    PluginHomeFixture Home(Project);
    Project.writePlugin("coditary",
                        "demo",
                        "1.0.0",
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
require {
    beez = {
        { name = "demo", version = "1.0.0" },
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
    PluginHomeFixture Home(Project);
    Project.writePlugin("unnamed",
                        "lazy",
                        "2.0.0",
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
require {
    beez = {
        { name = "lazy", version = "2.0.0" },
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
    PluginHomeFixture Home(Project);
    Project.writePlugin("unnamed",
                        "evil",
                        "1.0.0",
                        R"(
task("hack", "echo hacked")
plugin("evil", {
    version = "1.0.0",
    steps = {},
})
)");

    Project.writeBuildLua(R"(
require {
    beez = {
        { name = "evil", version = "1.0.0" },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsMissingPlugin)
{
    const beez::test::TempProject Project;
    PluginHomeFixture Home(Project);
    Project.writeBuildLua(R"(
require {
    beez = {
        { name = "missing", version = "1.0.0" },
    },
}
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, StringRequireStillWorks)
{
    const beez::test::TempProject Project;
    PluginHomeFixture Home(Project);
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

TEST(LuaDslPluginTest, LoadsPluginStepWithConfigAndLuaRun)
{
    const beez::test::TempProject Project;
    PluginHomeFixture Home(Project);
    Project.writePlugin("coditary",
                        "rich",
                        "1.0.0",
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
require {
    beez = {
        { name = "rich", version = "1.0.0" },
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
