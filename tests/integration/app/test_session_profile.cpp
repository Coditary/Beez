#include "beez/cli/session.hpp"
#include "beez/core/config/paths/config_paths.hpp"
#include "beez/core/util/temp_directory.hpp"
#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace
{

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
            return;
        }

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

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

  private:
    std::string name_;
    bool hadValue_ = false;
    bool unset_ = false;
    std::string saved_;
};

class ScopedTempTree
{
  public:
    explicit ScopedTempTree(std::filesystem::path path) : path_(std::move(path)) {}

    ~ScopedTempTree()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(path_, errorCode);
    }

    ScopedTempTree(const ScopedTempTree&) = delete;
    ScopedTempTree& operator=(const ScopedTempTree&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

  private:
    std::filesystem::path path_;
};

class ProfileTestEnv
{
  public:
    ProfileTestEnv()
        : root_(beez::core::systemTempDirectory() / "beez_integration_profile_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str()), cleanup_(root_)
    {
        std::filesystem::create_directories(configDir());
        std::filesystem::create_directories(profilesDir());
    }

    [[nodiscard]] std::filesystem::path configDir() const { return root_ / "xdg" / "beez"; }
    [[nodiscard]] std::filesystem::path profilesDir() const { return configDir() / "profiles"; }

    void writeGlobalConfig(const std::string& content) const
    {
        std::ofstream stream(configDir() / "config.lua");
        stream << content;
    }

    void writeProfile(const std::string& name, const std::string& content) const
    {
        std::ofstream stream(profilesDir() / (name + ".lua"));
        stream << content;
    }

  private:
    std::filesystem::path root_;
    ScopedEnv xdgEnv_;
    ScopedTempTree cleanup_;
};

[[nodiscard]] beez::cli::LoadedProject makeLoadedProject(const std::filesystem::path& projectRoot)
{
    beez::cli::LoadedProject project;
    project.context = beez::core::Context(projectRoot);
    return project;
}

}  // namespace

TEST(ProfileIntegrationTest, LoadGlobalSettingsWithoutProfile)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
    },
}
)");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    const auto Error = beez::cli::loadGlobalSettings(project);
    ASSERT_FALSE(Error.has_value()) << "loadGlobalSettings should succeed";

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 4U);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, LoadGlobalSettingsWithValidProfile)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
    },
    cache = {
        enabled = true,
    },
}
)");
    Env.writeProfile("dev", R"(
return {
    performance = {
        max_threads = 2,
    },
    cache = {
        enabled = false,
    },
}
)");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    const auto Error = beez::cli::loadGlobalSettings(project, "dev");
    ASSERT_FALSE(Error.has_value()) << "loadGlobalSettings with profile should succeed";

    // Profile should override global settings
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 2U);
    ASSERT_TRUE(project.settings.cache.enabled.has_value());
    EXPECT_FALSE(*project.settings.cache.enabled);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, LoadGlobalSettingsWithMissingProfileReturnsError)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    const auto Error = beez::cli::loadGlobalSettings(project, "nonexistent");
    ASSERT_TRUE(Error.has_value()) << "should return error for missing profile";
    EXPECT_NE(*Error, 0);
}

TEST(ProfileIntegrationTest, ProfileSettingsAppliedToEnvironment)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    ui = {
        output_mode = "clean",
    },
}
)");
    Env.writeProfile("silent", R"(
return {
    ui = {
        output_mode = "silent",
    },
}
)");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "silent").has_value());

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.ui.outputMode.has_value());
    EXPECT_EQ(*project.settings.ui.outputMode, beez::logging::OutputMode::Silent);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, ProfileOverridesGlobalButProjectOverridesProfile)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 8,
    },
}
)");
    Env.writeProfile("dev", R"(
return {
    performance = {
        max_threads = 2,
    },
}
)");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
beez.config({
    performance = {
        max_threads = 4,
    },
})
task("noop", "true")
)");

    auto project = makeLoadedProject(Project.path());

    // Load global + profile
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());

    // Profile overrides global
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 2U);
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Load build script and merge project settings
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());
    beez::cli::mergeProjectSettings(project, {});

    // Project overrides profile
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 4U);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, NoProfileLeavesGlobalSettingsUnchanged)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
    },
    cache = {
        enabled = true,
    },
}
)");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadGlobalSettings(project).has_value());

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 4U);
    ASSERT_TRUE(project.settings.cache.enabled.has_value());
    EXPECT_TRUE(*project.settings.cache.enabled);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, ProfileWithPartialOverrides)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
    },
    cache = {
        enabled = true,
        protect = false,
    },
    ui = {
        output_mode = "clean",
    },
}
)");
    Env.writeProfile("dev", R"(
return {
    cache = {
        enabled = false,
    },
}
)");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());

    // Only cache.enabled should be overridden, rest stays from global
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(project.settings.performance.maxThreads.has_value());
    EXPECT_EQ(*project.settings.performance.maxThreads, 4U);
    ASSERT_TRUE(project.settings.cache.enabled.has_value());
    EXPECT_FALSE(*project.settings.cache.enabled);
    ASSERT_TRUE(project.settings.cache.protect.has_value());
    EXPECT_FALSE(*project.settings.cache.protect);
    ASSERT_TRUE(project.settings.ui.outputMode.has_value());
    EXPECT_EQ(*project.settings.ui.outputMode, beez::logging::OutputMode::Clean);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(ProfileIntegrationTest, RegistryProfileSetFromLoadGlobalSettings)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("production", "return {}");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "production").has_value());

    ASSERT_TRUE(project.registry.profile().has_value());
    EXPECT_EQ(*project.registry.profile(), "production");
}

TEST(ProfileIntegrationTest, RegistryProfileNulloptWithoutProfile)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadGlobalSettings(project).has_value());

    EXPECT_FALSE(project.registry.profile().has_value());
}

TEST(ProfileIntegrationTest, PluginWithProfileIncludedWhenActive)
{
    beez::core::Registry registry;
    registry.setProfile("production");

    EXPECT_TRUE(registry.isPluginIncludedInProfile(std::string("production")));
}

TEST(ProfileIntegrationTest, PluginWithProfileExcludedWhenDifferent)
{
    beez::core::Registry registry;
    registry.setProfile("production");

    EXPECT_FALSE(registry.isPluginIncludedInProfile(std::string("dev")));
}

TEST(ProfileIntegrationTest, PluginWithNoneExcludedWhenProfileActive)
{
    beez::core::Registry registry;
    registry.setProfile("production");

    EXPECT_FALSE(registry.isPluginIncludedInProfile(std::string("NONE")));
}

TEST(ProfileIntegrationTest, PluginWithNoneIncludedWhenNoProfile)
{
    beez::core::Registry registry;

    EXPECT_TRUE(registry.isPluginIncludedInProfile(std::string("NONE")));
}

TEST(ProfileIntegrationTest, PluginWithNilAlwaysIncluded)
{
    beez::core::Registry registry;
    registry.setProfile("production");

    EXPECT_TRUE(registry.isPluginIncludedInProfile(std::nullopt));
}

TEST(ProfileIntegrationTest, ConfigurePluginProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;

    const auto PluginDir = Project.path() / "plugins" / "coditary" / "demo" / "1.0.0";
    std::filesystem::create_directories(PluginDir);
    std::ofstream(PluginDir / "beez_plugin.lua") << R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = "echo check",
        },
    },
})
)";

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
    }, profile = "dev" },
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_NE(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigurePluginProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::TempProject Project;

    const auto PluginDir = Project.path() / "plugins" / "coditary" / "demo" / "1.0.0";
    std::filesystem::create_directories(PluginDir);
    std::ofstream(PluginDir / "beez_plugin.lua") << R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = "echo check",
        },
    },
})
)";

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
    }, profile = "dev" },
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "test").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_EQ(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigurePluginWithoutProfileAlwaysApplied)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;

    const auto PluginDir = Project.path() / "plugins" / "coditary" / "demo" / "1.0.0";
    std::filesystem::create_directories(PluginDir);
    std::ofstream(PluginDir / "beez_plugin.lua") << R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = "echo check",
        },
    },
})
)";

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
    } },
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_NE(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigurePluginNoneProfileAppliedWhenNoActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;

    const auto PluginDir = Project.path() / "plugins" / "coditary" / "demo" / "1.0.0";
    std::filesystem::create_directories(PluginDir);
    std::ofstream(PluginDir / "beez_plugin.lua") << R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = "echo check",
        },
    },
})
)";

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
    }, profile = "NONE" },
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project).has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_NE(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigurePluginNoneProfileSkippedWhenActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;

    const auto PluginDir = Project.path() / "plugins" / "coditary" / "demo" / "1.0.0";
    std::filesystem::create_directories(PluginDir);
    std::ofstream(PluginDir / "beez_plugin.lua") << R"(
plugin("demo", {
    version = "1.0.0",
    steps = {
        check = {
            phase = "test",
            scope = "code",
            config = { flag = "base" },
            run = "echo check",
        },
    },
})
)";

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
    }, profile = "NONE" },
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("check");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_EQ(Found->config->cacheFingerprint().find("compdb=build/tree"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigureStepProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure({
    { ":clean:artifacts", {
        force = true,
    }, profile = "dev" },
})

step({
    name = "clean:artifacts",
    phase = "clean",
    scope = "repo",
    run = "true",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("clean:artifacts");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_NE(Found->config->cacheFingerprint().find("force=true"), std::string::npos);
}

TEST(ProfileIntegrationTest, ConfigureStepProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
configure({
    { ":clean:artifacts", {
        force = true,
    }, profile = "dev" },
})

step({
    name = "clean:artifacts",
    phase = "clean",
    scope = "repo",
    run = "true",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "test").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findStep("clean:artifacts");
    ASSERT_TRUE(Found.has_value());
    if (!Found || Found->config == nullptr)
    {
        return;
    }

    EXPECT_EQ(Found->config->cacheFingerprint().find("force=true"), std::string::npos);
}

TEST(ProfileIntegrationTest, TaskWithProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("debug", {
    profile = "dev",
    "echo debug",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findTask("debug");
    ASSERT_TRUE(Found.has_value());
}

TEST(ProfileIntegrationTest, TaskWithProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("debug", {
    profile = "dev",
    "echo debug",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "test").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findTask("debug");
    EXPECT_FALSE(Found.has_value());
}

TEST(ProfileIntegrationTest, TaskWithoutProfileAlwaysApplied)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("debug", {
    "echo debug",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findTask("debug");
    ASSERT_TRUE(Found.has_value());
}

TEST(ProfileIntegrationTest, TaskWithNoneProfileAppliedWhenNoActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("debug", {
    profile = "NONE",
    "echo debug",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project).has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findTask("debug");
    ASSERT_TRUE(Found.has_value());
}

TEST(ProfileIntegrationTest, TaskWithNoneProfileSkippedWhenActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("debug", {
    profile = "NONE",
    "echo debug",
})
)");

    auto project = makeLoadedProject(Project.path());
    ASSERT_FALSE(beez::cli::loadGlobalSettings(project, "dev").has_value());
    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());

    const auto Found = project.registry.findTask("debug");
    EXPECT_FALSE(Found.has_value());
}
