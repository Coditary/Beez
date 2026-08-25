#include "helpers/fixture_project.hpp"
#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

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
        : root_(beez::test::testTempDirectory() / "beez_system_profile_env"),
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

}  // namespace

TEST(SystemProfileTest, ProfileFlagShowsErrorForMissingProfile)
{
    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "nonexistent", "hello"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "profile not found: nonexistent"));
}

TEST(SystemProfileTest, ProfileFlagWithShowConfig)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", R"(
return {
    performance = {
        max_threads = 2,
    },
}
)");

    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "performance.max_threads"));
    EXPECT_TRUE(beez::test::outputContains(Result, "2"));
}

TEST(SystemProfileTest, NoProfileRunsAllTasks)
{
    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("hello.out"));
}

TEST(SystemProfileTest, ProfileWithShowConfigShowsProfileSettings)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
    },
}
)");
    Env.writeProfile("production", R"(
return {
    performance = {
        max_threads = 8,
    },
}
)");

    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "production", "--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "performance.max_threads"));
    EXPECT_TRUE(beez::test::outputContains(Result, "8"));
}

TEST(SystemProfileTest, ProfileOverridesGlobalSettings)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
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

    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "--show-config"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "performance.max_threads"));
    EXPECT_TRUE(beez::test::outputContains(Result, "2"));
}

TEST(SystemProfileTest, DifferentProfilesShowDifferentSettings)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig(R"(
return {
    performance = {
        max_threads = 4,
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
    Env.writeProfile("production", R"(
return {
    performance = {
        max_threads = 8,
    },
}
)");

    const beez::test::FixtureProject Project("profile-basic");

    const beez::test::ProcessResult DevResult =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "--show-config"});
    EXPECT_EQ(DevResult.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(DevResult, "2"));

    const beez::test::ProcessResult ProdResult =
        beez::test::runBeez(Project.path(), {"--profile", "production", "--show-config"});
    EXPECT_EQ(ProdResult.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(ProdResult, "8"));
}

TEST(SystemProfileTest, TaskWithProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
task("debug", {
    profile = "dev",
    "echo debug > debug.out",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "debug"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("debug.out"));
}

TEST(SystemProfileTest, TaskWithProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
task("debug", {
    profile = "dev",
    "echo debug > debug.out",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "test", "debug"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "not found"));
    EXPECT_FALSE(Project.hasFile("debug.out"));
}

TEST(SystemProfileTest, TaskWithNoneProfileAppliedWhenNoActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
task("debug", {
    profile = "NONE",
    "echo debug > debug.out",
})
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"debug"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("debug.out"));
}

TEST(SystemProfileTest, TaskWithNoneProfileSkippedWhenActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
task("debug", {
    profile = "NONE",
    "echo debug > debug.out",
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "debug"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "not found"));
    EXPECT_FALSE(Project.hasFile("debug.out"));
}

TEST(SystemProfileTest, LocalStepWithProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({
    name = "custom-check",
    phase = "test",
    scope = "code",
    run = "echo check > check.out",
    profile = "dev",
})

task("run-check", {
    { step = "custom-check" },
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "run-check"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("check.out"));
}

TEST(SystemProfileTest, LocalStepWithProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({
    name = "custom-check",
    phase = "test",
    scope = "code",
    run = "echo check > check.out",
    profile = "dev",
})

task("run-check", {
    { step = "custom-check" },
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "test", "run-check"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_FALSE(Project.hasFile("check.out"));
}

TEST(SystemProfileTest, LocalStepWithNoneProfileAppliedWhenNoActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({
    name = "custom-check",
    phase = "test",
    scope = "code",
    run = "echo check > check.out",
    profile = "NONE",
})

task("run-check", {
    { step = "custom-check" },
})
)");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"run-check"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(Project.hasFile("check.out"));
}

TEST(SystemProfileTest, LocalStepWithNoneProfileSkippedWhenActive)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({
    name = "custom-check",
    phase = "test",
    scope = "code",
    run = "echo check > check.out",
    profile = "NONE",
})

task("run-check", {
    { step = "custom-check" },
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "run-check"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_FALSE(Project.hasFile("check.out"));
}

TEST(SystemProfileTest, LocalWorkflowWithProfileAppliedWhenMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({ name = "dev-build", phase = "build", scope = "dev", run = "echo dev > dev.out" })

workflow("ship", {
    profile = "dev",
    { "verify", { "build[dev]" } },
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "dev", "ship"});
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
    EXPECT_TRUE(Project.hasFile("dev.out"));
}

TEST(SystemProfileTest, LocalWorkflowWithProfileSkippedWhenNotMatching)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("test", "return {}");

    const beez::test::FixtureProject Project("profile-basic");
    Project.writeFile("build.lua", R"(
step({ name = "dev-build", phase = "build", scope = "dev", run = "echo dev > dev.out" })

workflow("ship", {
    profile = "dev",
    { "verify", { "build[dev]" } },
})
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--profile", "test", "ship"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "not found"));
    EXPECT_FALSE(Project.hasFile("dev.out"));
}

TEST(SystemProfileTest, WorkflowsReferenceSelectsPluginWorkflowByProfile)
{
    const ProfileTestEnv Env;
    Env.writeGlobalConfig("return {}");
    Env.writeProfile("dev", "return {}");

    const char* BuildLua = R"(
step({ name = "dev-build", phase = "build", scope = "dev", run = "echo dev > dev.out" })
step({ name = "base-build", phase = "build", scope = "base", run = "echo base > base.out" })

reqpack {
    beez = {
        {
            name = "coditary/demo",
            path = "./plugins/coditary/demo",
            version = "1.0.0",
        },
    },
}

workflows({ ship = {
    reference = "coditary/demo:ship-dev",
    profile = "dev",
} })
workflows({ ship = {
    reference = "coditary/demo:ship-base",
    profile = "NONE",
} })
)";

    const beez::test::FixtureProject DevProject("profile-basic");
    DevProject.writeFile("plugins/coditary/demo/1.0.0/beez_plugin.lua", R"(
plugin("demo", {
    version = "1.0.0",
    steps = {},
})

workflows {
    ["ship-dev"] = { "build[dev]" },
    ["ship-base"] = { "build[base]" },
}
)");
    DevProject.writeFile("build.lua", BuildLua);

    const beez::test::ProcessResult DevResult =
        beez::test::runBeez(DevProject.path(), {"--profile", "dev", "ship"});
    EXPECT_EQ(DevResult.exitCode, 0) << DevResult.output;
    EXPECT_TRUE(DevProject.hasFile("dev.out"));
    EXPECT_FALSE(DevProject.hasFile("base.out"));

    const beez::test::FixtureProject BaseProject("profile-basic");
    BaseProject.writeFile("plugins/coditary/demo/1.0.0/beez_plugin.lua", R"(
plugin("demo", {
    version = "1.0.0",
    steps = {},
})

workflows {
    ["ship-dev"] = { "build[dev]" },
    ["ship-base"] = { "build[base]" },
}
)");
    BaseProject.writeFile("build.lua", BuildLua);

    const beez::test::ProcessResult BaseResult =
        beez::test::runBeez(BaseProject.path(), {"ship"});
    EXPECT_EQ(BaseResult.exitCode, 0) << BaseResult.output;
    EXPECT_FALSE(BaseProject.hasFile("dev.out"));
    EXPECT_TRUE(BaseProject.hasFile("base.out"));
}
