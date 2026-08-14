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

TEST(LuaDslPluginTest, ConfigurePluginMergesIntoAllPluginSteps)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        alpha = {
            phase = "qa",
            scope = "code",
            config = { flag = "base" },
            run = "echo alpha",
        },
        beta = {
            phase = "qa",
            scope = "lint",
            config = { flag = "base" },
            run = "echo beta",
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

configure_plugin("coditary/demo", {
    compdb = "build/tree",
})

configure_step("alpha", {
    flag = "alpha",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Alpha = registry.findStep("alpha");
    const auto Beta = registry.findStep("beta");
    ASSERT_TRUE(Alpha.has_value());
    ASSERT_TRUE(Beta.has_value());
    if (!Alpha || !Beta)
    {
        return;
    }

    ASSERT_TRUE(Alpha->hasConfig());
    ASSERT_TRUE(Beta->hasConfig());
    if (Alpha->config == nullptr || Beta->config == nullptr)
    {
        return;
    }

    const std::string AlphaFingerprint = Alpha->config->cacheFingerprint();
    const std::string BetaFingerprint = Beta->config->cacheFingerprint();
    EXPECT_NE(AlphaFingerprint.find("compdb=build/tree"), std::string::npos);
    EXPECT_NE(BetaFingerprint.find("compdb=build/tree"), std::string::npos);
    EXPECT_NE(AlphaFingerprint.find("flag=alpha"), std::string::npos);
    EXPECT_NE(BetaFingerprint.find("flag=base"), std::string::npos);
}

TEST(LuaDslPluginTest, ConfigurePluginBeforeReqpackAppliesOnRegistration)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "qa",
            scope = "code",
            run = "echo check",
        },
    },
})
)");

    Project.writeBuildLua(R"(
configure_plugin("coditary/demo", {
    compdb = "build/tree",
})

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

    const auto Found = registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_NE(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(LuaDslPluginTest, RejectsConfigurePluginNotInReqpack)
{
    const beez::test::TempProject Project;
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

configure_plugin("coditary/missing", {
    compdb = "build/tree",
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
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

configure_plugin("coditary/merge", {
    compdb = "build/tree",
})

configure_step("check", {
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

TEST(LuaDslPluginTest, ConfigureMergesPluginAndStepConfigs)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        alpha = {
            phase = "qa",
            scope = "code",
            config = { flag = "base" },
            run = "echo alpha",
        },
        beta = {
            phase = "qa",
            scope = "lint",
            config = { flag = "base" },
            run = "echo beta",
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

configure({
    { "coditary/demo", {
        compdb = "build/tree",
        steps = {
            alpha = { flag = "alpha" },
        },
    }},
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Alpha = registry.findStep("alpha");
    const auto Beta = registry.findStep("beta");
    ASSERT_TRUE(Alpha.has_value());
    ASSERT_TRUE(Beta.has_value());
    if (!Alpha || !Beta)
    {
        return;
    }

    ASSERT_TRUE(Alpha->hasConfig());
    ASSERT_TRUE(Beta->hasConfig());
    if (Alpha->config == nullptr || Beta->config == nullptr)
    {
        return;
    }

    const std::string AlphaFingerprint = Alpha->config->cacheFingerprint();
    const std::string BetaFingerprint = Beta->config->cacheFingerprint();
    EXPECT_NE(AlphaFingerprint.find("compdb=build/tree"), std::string::npos);
    EXPECT_NE(BetaFingerprint.find("compdb=build/tree"), std::string::npos);
    EXPECT_NE(AlphaFingerprint.find("flag=alpha"), std::string::npos);
    EXPECT_NE(BetaFingerprint.find("flag=base"), std::string::npos);
}

TEST(LuaDslPluginTest, RejectsConfigurePluginNotInReqpackViaConfigure)
{
    const beez::test::TempProject Project;
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

configure({
    { "coditary/missing", {
        compdb = "build/tree",
    }},
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, ConfigureStandaloneStepBeforeRegistration)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure({
    { ":clean:artifacts", {
        force = true,
        delete_logs = false,
    }},
})

step({
    name = "clean:artifacts",
    phase = "clean",
    scope = "repo",
    run = "true",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findStep("clean:artifacts");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    const std::string Fingerprint = Found->config->cacheFingerprint();
    EXPECT_NE(Fingerprint.find("force=true"), std::string::npos);
    EXPECT_NE(Fingerprint.find("delete_logs=false"), std::string::npos);
}

TEST(LuaDslPluginTest, RejectsConfigureStandaloneStepWithEmptyName)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure({
    { ":", {
        force = true,
    }},
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, PluginWorkflowsAreStoredWithoutAutoRegistration)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        configure_release = {
            phase = "configure",
            scope = "release",
            run = "echo configure",
        },
        build_release = {
            phase = "build",
            scope = "release",
            run = "echo build",
        },
    },
})

workflows {
    release = {
        "configure:release",
        "build:release",
    },
}
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

    EXPECT_FALSE(registry.findWorkflow("release").has_value());
    EXPECT_EQ(registry.pluginWorkflows().size(), 1U);
    EXPECT_TRUE(registry.pluginWorkflows().contains("coditary/demo:release"));
}

TEST(LuaDslPluginTest, WorkflowShorthandImportsPluginWorkflow)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        configure_release = {
            phase = "configure",
            scope = "release",
            run = "echo configure",
        },
    },
})

workflows {
    release = {
        "configure:release",
    },
}
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

workflow("ship_it", "coditary/demo:release")
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findWorkflow("ship_it");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }

    ASSERT_EQ(Found->steps.size(), 1U);
    EXPECT_EQ(Found->steps[0].invocation.phase, "configure");
    EXPECT_EQ(Found->steps[0].invocation.scope, "release");
}

TEST(LuaDslPluginTest, WorkflowsBatchImportsPluginWorkflow)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        test_code = {
            phase = "test",
            scope = "code",
            run = "echo test",
        },
    },
})

workflows {
    verify = {
        "test:code",
    },
}
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

workflows({
    release = "coditary/demo:verify",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findWorkflow("release");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }

    ASSERT_EQ(Found->steps.size(), 1U);
    EXPECT_EQ(Found->steps[0].invocation.phase, "test");
    EXPECT_EQ(Found->steps[0].invocation.scope, "code");
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

TEST(LuaDslPluginTest, LoadsTaskWithPluginStepFieldForm)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        compile = {
            phase = "compile",
            scope = "code",
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

task("build", {
    {
        plugin = "coditary/demo",
        step = "compile",
    },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "build");
    ASSERT_TRUE(Found.has_value());
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectStepInvocation(Found, 0, "coditary/demo:compile", false);
}

TEST(LuaDslPluginTest, LoadsTaskWithPluginStepAndScopeFields)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        ["configure:debug"] = {
            phase = "configure",
            scope = "debug",
            run = "echo configure",
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

task("build", {
    {
        plugin = "coditary/demo",
        step = "configure[debug]",
    },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "build");
    ASSERT_TRUE(Found.has_value());
    ASSERT_EQ(Found->actions.size(), 1U);
    beez::test::expectStepInvocation(Found, 0, "coditary/demo:configure:debug", false);
}

TEST(LuaDslPluginTest, ExpandsTaskStepWithMultipleScopes)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        ["configure:debug"] = {
            phase = "configure",
            scope = "debug",
            run = "echo debug",
        },
        ["configure:fuzzer"] = {
            phase = "configure",
            scope = "fuzzer",
            run = "echo fuzzer",
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

task("build", {
    {
        plugin = "coditary/demo",
        step = "configure[\"debug\", \"fuzzer\"]",
    },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "build");
    ASSERT_TRUE(Found.has_value());
    ASSERT_EQ(Found->actions.size(), 2U);
    beez::test::expectStepInvocation(Found, 0, "coditary/demo:configure:debug", false);
    beez::test::expectStepInvocation(Found, 1, "coditary/demo:configure:fuzzer", false);
}

TEST(LuaDslPluginTest, RejectsDeprecatedScopeFieldInTaskAction)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("build", {
    { plugin = "coditary/demo", step = "configure", scope = "debug" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, LoadsTaskWithTaskInvocation)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("prepare", "echo prepare")
task("build", {
    { task = "prepare" },
    "echo done",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "build");
    ASSERT_TRUE(Found.has_value());
    ASSERT_EQ(Found->actions.size(), 2U);
    beez::test::expectTaskInvocation(Found, 0, "prepare");
}

TEST(LuaDslPluginTest, RejectsBarePluginStepNameInTask)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        compile = {
            phase = "compile",
            scope = "code",
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

task("build", {
    { step = "compile" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsPluginStepNotDeclaredInReqpack)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        compile = {
            phase = "compile",
            scope = "code",
            run = "echo demo",
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

task("build", {
    {
        plugin = "coditary/other",
        step = "compile",
    },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsDeprecatedNameFieldInTaskAction)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "local-step",
    phase = "build",
    scope = "code",
    run = "echo local",
})
task("build", {
    { name = "local-step" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsVersionFieldInTaskAction)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        compile = {
            phase = "compile",
            scope = "code",
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

task("build", {
    {
        plugin = "coditary/demo",
        step = "compile",
        version = "1.0.0",
    },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsDirectTaskCycle)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("loop", {
    { task = "loop" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsIndirectTaskCycle)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("a", {
    { task = "b" },
})
task("b", {
    { task = "a" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsTaskActionWithPhaseAndStep)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "local-step",
    phase = "build",
    scope = "code",
    run = "echo local",
})
task("build", {
    { phase = "build[code]", step = "local-step" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, RejectsUndefinedTaskInvocation)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("build", {
    { task = "missing" },
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, PipelineStandardWorkflowDefinesUnscopedStages)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/pipeline/1.0.0",
                          R"(
plugin("pipeline", {
    version = "1.0.0",
    steps = {},
})

workflows {
    standard = {
        { "setup", { "setup" } },
        { "generate", { "generate" } },
        { "quality", { "quality" } },
        { "compile", { "compile" } },
        { "bundle", { "bundle" } },
        { "test", { "test" } },
        { "package", { "package" } },
        { "verify", { "verify" } },
        { "publish", { "publish" } },
    },
}
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/pipeline",
            path = "./plugins/coditary/pipeline",
            version = "1.0.0",
        },
    },
}
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    ASSERT_TRUE(registry.pluginWorkflows().contains("coditary/pipeline:standard"));
    const auto& Workflow = registry.pluginWorkflows().at("coditary/pipeline:standard");
    ASSERT_TRUE(Workflow.isStaged());
    ASSERT_EQ(Workflow.stages.size(), 9U);

    const std::vector<std::string> ExpectedStages = {
        "setup", "generate", "quality", "compile", "bundle", "test", "package", "verify", "publish",
    };
    for (std::size_t index = 0; index < ExpectedStages.size(); ++index)
    {
        EXPECT_EQ(Workflow.stages[index].name, ExpectedStages[index]);
        ASSERT_EQ(Workflow.stages[index].invocations.size(), 1U);
        EXPECT_EQ(Workflow.stages[index].invocations[0].phase, ExpectedStages[index]);
        EXPECT_TRUE(Workflow.stages[index].invocations[0].scope.empty());
    }
}

TEST(LuaDslPluginTest, RejectsImportedPluginWorkflowWithoutMatchingSteps)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/pipeline/1.0.0",
                          R"(
plugin("pipeline", {
    version = "1.0.0",
    steps = {},
})

workflows {
    standard = {
        { "setup", { "setup" } },
    },
}
)");

    Project.writeBuildLua(R"(
reqpack {
    beez = {
        {
            name = "coditary/pipeline",
            path = "./plugins/coditary/pipeline",
            version = "1.0.0",
        },
    },
}

workflows({
    standard = "coditary/pipeline:standard",
})
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaDslPluginTest, TaskShorthandImportsPluginTask)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        greet = {
            phase = "quality",
            scope = "lint",
            run = "echo hello",
        },
    },
})

tasks {
    format = {
        { plugin = "coditary/demo", step = "greet" },
    },
}
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

task("coditary/demo:format")
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    ASSERT_TRUE(registry.pluginTasks().contains("coditary/demo:format"));
    const auto Found = registry.findTask("format");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }

    ASSERT_EQ(Found->actions.size(), 1U);
}

TEST(LuaDslPluginTest, TaskImportTableAliasesPluginTask)
{
    const beez::test::TempProject Project;
    Project.writePluginAt("plugins/coditary/demo/1.0.0",
                          R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        greet = {
            phase = "quality",
            scope = "lint",
            run = "echo hello",
        },
    },
})

tasks {
    format = {
        { plugin = "coditary/demo", step = "greet" },
    },
}
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

task("formater", {
    plugin = "coditary/demo",
    task = "format",
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findTask("formater");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }

    EXPECT_EQ(Found->name, "formater");
    ASSERT_EQ(Found->actions.size(), 1U);
}
