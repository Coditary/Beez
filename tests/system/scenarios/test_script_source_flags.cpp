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

class SourceFlagSystemEnv
{
  public:
    SourceFlagSystemEnv()
        : root_(beez::test::testTempDirectory() / "beez_system_source_flag_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str())
    {
        std::filesystem::remove_all(root_);
        std::filesystem::create_directories(root_ / "xdg" / "beez");
    }

    ~SourceFlagSystemEnv()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(root_, errorCode);
    }

    SourceFlagSystemEnv(const SourceFlagSystemEnv&) = delete;
    SourceFlagSystemEnv& operator=(const SourceFlagSystemEnv&) = delete;

    void writeGlobalBuildLua(const std::string& content) const
    {
        const auto GlobalDir = root_ / "xdg" / "beez" / "global";
        std::filesystem::create_directories(GlobalDir);
        std::ofstream stream(GlobalDir / "build.lua");
        stream << content;
    }

  private:
    std::filesystem::path root_;
    ScopedEnv xdgEnv_;
};

}  // namespace

TEST(SystemSourceFlagsTest, BridgeFlagRunsLinkedVersionInsteadOfLocal)
{
    const SourceFlagSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"hello\", \"echo from-v1\")\n");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--link"}).exitCode, 0);

    Project.writeBuildLua("task(\"hello\", \"echo from-v2\")\n");

    const beez::test::ProcessResult LocalRun = beez::test::runBeez(Project.path(), {"hello"});
    EXPECT_EQ(LocalRun.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(LocalRun, "from-v2"));

    const beez::test::ProcessResult BridgeRun = beez::test::runBeez(Project.path(), {"-l", "hello"});
    EXPECT_EQ(BridgeRun.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(BridgeRun, "from-v1"));
}

TEST(SystemSourceFlagsTest, GlobalFlagRunsGlobalScriptInsteadOfLocal)
{
    const SourceFlagSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"gtask\", \"echo global-run-ok\")\n");

    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"local-only\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-g", "gtask"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "global-run-ok"));
}

TEST(SystemSourceFlagsTest, FlagsWorkAfterTargetPosition)
{
    const SourceFlagSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"gtask\", \"echo global-after-target\")\n");

    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"hello\", \"true\")\n");
    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--link"}).exitCode, 0);

    const beez::test::ProcessResult BridgeAfter =
        beez::test::runBeez(Project.path(), {"hello", "-l"});
    EXPECT_EQ(BridgeAfter.exitCode, 0);

    const beez::test::ProcessResult GlobalAfter =
        beez::test::runBeez(Project.path(), {"gtask", "-g"});
    EXPECT_EQ(GlobalAfter.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(GlobalAfter, "global-after-target"));
}

TEST(SystemSourceFlagsTest, BridgeFlagFailsWithoutLink)
{
    const SourceFlagSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"hello\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-l", "hello"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "no bridge linked"));
}

TEST(SystemSourceFlagsTest, GlobalFlagFailsWithoutGlobalScript)
{
    const SourceFlagSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"hello\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"-g", "hello"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "global build script not found"));
}

TEST(SystemSourceFlagsTest, BridgeAndGlobalFlagsTogetherFail)
{
    const SourceFlagSystemEnv Env;
    Env.writeGlobalBuildLua("task(\"gtask\", \"true\")\n");

    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"hello\", \"true\")\n");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"-l", "-g", "hello"});
    EXPECT_NE(Result.exitCode, 0);
}
