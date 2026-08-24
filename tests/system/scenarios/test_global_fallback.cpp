#include "helpers/process_runner.hpp"
#include "helpers/scratch_project.hpp"
#include "temp_directory.hpp"

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

class FallbackSystemEnv
{
  public:
    FallbackSystemEnv()
        : root_(beez::test::testTempDirectory() / "beez_system_fallback_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str())
    {
        std::filesystem::remove_all(root_);
        std::filesystem::create_directories(root_ / "xdg" / "beez");
    }

    ~FallbackSystemEnv()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(root_, errorCode);
    }

    FallbackSystemEnv(const FallbackSystemEnv&) = delete;
    FallbackSystemEnv& operator=(const FallbackSystemEnv&) = delete;

    [[nodiscard]] std::filesystem::path globalDir() const
    {
        return root_ / "xdg" / "beez" / "global";
    }

    void writeGlobalBuildLua(const std::string& content) const
    {
        std::filesystem::create_directories(globalDir());
        std::ofstream stream(globalDir() / "build.lua");
        stream << content;
    }

  private:
    std::filesystem::path root_;
    ScopedEnv xdgEnv_;
};

}  // namespace

TEST(SystemGlobalFallbackTest, GlobalBuildLuaUsedWhenNoLocalAndNoBridge)
{
    const FallbackSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"global-task\", \"echo global-fallback-ok\")\n");

    const beez::test::ScratchProject Project;

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"global-task"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "global-fallback-ok"));
}

TEST(SystemGlobalFallbackTest, LocalBuildLuaWinsOverGlobal)
{
    const FallbackSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"shared-name\", \"echo from-global\")\n");

    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"shared-name\", \"echo from-local\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"shared-name"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "from-local"));
    EXPECT_FALSE(beez::test::outputContains(Result, "from-global"));
}

TEST(SystemGlobalFallbackTest, BridgeWinsOverGlobal)
{
    const FallbackSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"shared-name\", \"echo from-global\")\n");

    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"shared-name\", \"echo from-bridge\")\n");
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--link"}).exitCode, 0);

    std::error_code errorCode;
    std::filesystem::remove(Project.path() / "build.lua", errorCode);

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"shared-name"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "from-bridge"));
    EXPECT_FALSE(beez::test::outputContains(Result, "from-global"));
}

TEST(SystemGlobalFallbackTest, FailsWhenNoScriptAnywhere)
{
    const FallbackSystemEnv Env;
    const beez::test::ScratchProject Project;

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"noop"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "build script not found"));
}
