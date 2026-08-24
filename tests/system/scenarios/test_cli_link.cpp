#include "helpers/process_runner.hpp"
#include "helpers/scratch_project.hpp"
#include "temp_directory.hpp"

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

class LinkSystemEnv
{
  public:
    LinkSystemEnv()
        : root_(beez::test::testTempDirectory() / "beez_system_link_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str())
    {
        std::filesystem::remove_all(root_);
        std::filesystem::create_directories(root_ / "xdg" / "beez");
    }

    ~LinkSystemEnv()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(root_, errorCode);
    }

    LinkSystemEnv(const LinkSystemEnv&) = delete;
    LinkSystemEnv& operator=(const LinkSystemEnv&) = delete;

    [[nodiscard]] std::filesystem::path bridgesDir() const
    {
        return root_ / "xdg" / "beez" / "bridges";
    }

    [[nodiscard]] std::filesystem::path indexPath() const { return bridgesDir() / "index.json"; }

  private:
    std::filesystem::path root_;
    ScopedEnv xdgEnv_;
};

}  // namespace

TEST(SystemCliLinkTest, LinkCreatesBridgeAndReportsPath)
{
    const LinkSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--link"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "Linked"));
    EXPECT_TRUE(beez::test::outputContains(Result, "Bridge:"));
    EXPECT_TRUE(std::filesystem::exists(Env.indexPath()));
}

TEST(SystemCliLinkTest, RelinkingExistingBridgeShowsWarning)
{
    const LinkSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"noop\", \"true\")\n");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--link"}).exitCode, 0);

    const beez::test::ProcessResult Second = beez::test::runBeez(Project.path(), {"--link"});
    EXPECT_EQ(Second.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Second, "Warning: bridge already exists"));
}

TEST(SystemCliLinkTest, LinkWithoutBuildScriptFails)
{
    const LinkSystemEnv Env;
    const beez::test::ScratchProject Project;

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"--link"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "Error: build script not found"));
    EXPECT_FALSE(std::filesystem::exists(Env.indexPath()));
}

TEST(SystemCliLinkTest, TaskRunsThroughBridgeAfterLocalBuildLuaDeleted)
{
    const LinkSystemEnv Env;
    const beez::test::ScratchProject Project;
    Project.writeBuildLua("task(\"bridge-task\", \"echo bridge-system-ok\")\n");

    ASSERT_EQ(beez::test::runBeez(Project.path(), {"--link"}).exitCode, 0);

    std::error_code errorCode;
    std::filesystem::remove(Project.path() / "build.lua", errorCode);
    ASSERT_FALSE(std::filesystem::exists(Project.path() / "build.lua"));

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"bridge-task"});
    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "bridge-system-ok"));
}

TEST(SystemCliLinkTest, UnlinkedProjectWithoutBuildLuaStillFails)
{
    const LinkSystemEnv Env;
    const beez::test::ScratchProject Project;

    const beez::test::ProcessResult Result = beez::test::runBeez(Project.path(), {"noop"});
    EXPECT_NE(Result.exitCode, 0);
    EXPECT_TRUE(beez::test::outputContains(Result, "build script not found"));
}
