#include "beez/core/registry/registry.hpp"
#include "beez/core/registry/step_resolution.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/temp_directory.hpp"
#include "beez/plugin/lua/dsl/step_plugin_loader.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <string>

namespace
{

// NOLINTBEGIN(misc-include-cleaner,concurrency-mt-unsafe,cert-env33-c,bugprone-command-processor)
class ScopedEnv
{
  public:
    ScopedEnv(const char* name, const char* value) : name_(name)
    {
        if (const char* existing = std::getenv(name); existing != nullptr)
        {
            hadValue_ = true;
            saved_ = existing;
        }

        if (value[0] == '\0')
        {
            unsetenv(name);
            unset_ = true;
        }
        else
        {
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
                setenv(name_.c_str(), saved_.c_str(), 1);
            }
            else
            {
                unsetenv(name_.c_str());
            }
        }
        else if (hadValue_)
        {
            setenv(name_.c_str(), saved_.c_str(), 1);
        }
    }

  private:
    std::string name_;
    bool hadValue_ = false;
    bool unset_ = false;
    std::string saved_;
};
// NOLINTEND(misc-include-cleaner,concurrency-mt-unsafe,cert-env33-c,bugprone-command-processor)

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

TEST(LuaDslPluginTest, ConfigureStepMergesOverlayIntoPluginStepConfig)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/merge/1.0.0",
                          R"(
plugin("merge", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = function(ctx)
                local config = ctx.get_config()
                if config.flag ~= "overlay" then
                    return 1
                end
                if config.compdb ~= "build/tree" then
                    return 2
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
            name = "coditary/merge",
            path = "./plugins/coditary/merge",
            version = "1.0.0",
        },
    },
}

configure_step("check", {
    compdb = "build/tree",
    flag = "overlay",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }

    ASSERT_TRUE(Found->hasConfig());
    EXPECT_NE(Found->config, nullptr);
    if (Found->config == nullptr)
    {
        return;
    }
    const std::string Fingerprint = Found->config->cacheFingerprint();
    EXPECT_NE(Fingerprint.find("compdb=build/tree"), std::string::npos);
    EXPECT_NE(Fingerprint.find("flag=overlay"), std::string::npos);
}

TEST(LuaDslPluginTest, LoadsInstalledPluginVersionOnDemand)
{
    const beez::test::TempProject Project;
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_installed_plugin_test";
    std::error_code errorCode;
    std::filesystem::remove_all(HomeRoot, errorCode);
    std::filesystem::create_directories(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CACHE_HOME", "");

    const auto PluginPath = HomeRoot / ".cache" / "beez" / "plugins" / "coditary" / "remote" /
                            "1.0.0" / "beez_plugin.lua";
    std::filesystem::create_directories(PluginPath.parent_path());
    std::ofstream(PluginPath) << R"(
plugin("remote", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "qa",
            scope = "code",
            run = "echo remote",
        },
    },
})
)";

    Project.writeBuildLua(R"(
task("noop", "true")
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto EnsureResult = beez::plugin::lua::ensureInstalledPluginForStepReference(
        "coditary/remote:check@1.0.0", registry, beez::core::Context(Project.path()));
    ASSERT_TRUE(EnsureResult.success) << EnsureResult.message;

    const auto Resolved = registry.resolveStep("coditary/remote:check@1.0.0");
    ASSERT_TRUE(Resolved.hasValue());
    ASSERT_TRUE(Resolved.value().hasShellRun());
    EXPECT_EQ(Resolved.value().shellRun.value_or(""), "echo remote");

    std::filesystem::remove_all(HomeRoot, errorCode);
}

TEST(LuaDslPluginTest, ResolvesDuplicatePluginStepNamesWithQualifiedReference)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/tidy/1.0.0",
                          R"(
plugin("tidy", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "qa",
            scope = "code",
            run = "echo tidy",
        },
    },
})
)");
    Project.writePluginAt("plugins/coditary/cppcheck/1.0.0",
                          R"(
plugin("cppcheck", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "qa",
            scope = "code",
            run = "echo cppcheck",
        },
    },
})
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/tidy",
            path = "./plugins/coditary/tidy",
            version = "1.0.0",
        },
        {
            name = "coditary/cppcheck",
            path = "./plugins/coditary/cppcheck",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Tidy = registry.resolveStep("coditary/tidy:check");
    ASSERT_TRUE(Tidy.hasValue());
    ASSERT_TRUE(Tidy.value().hasShellRun());
    EXPECT_EQ(Tidy.value().shellRun.value_or(""), "echo tidy");

    const auto Cppcheck = registry.resolveStep("cppcheck:check");
    ASSERT_TRUE(Cppcheck.hasValue());
    ASSERT_TRUE(Cppcheck.value().hasShellRun());
    EXPECT_EQ(Cppcheck.value().shellRun.value_or(""), "echo cppcheck");

    const auto Ambiguous = registry.resolveStep("check");
    ASSERT_FALSE(Ambiguous.hasValue());
    EXPECT_EQ(Ambiguous.error().error, beez::core::StepResolutionError::Ambiguous);
}
