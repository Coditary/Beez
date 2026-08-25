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
