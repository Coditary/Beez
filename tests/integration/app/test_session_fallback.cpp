#include "beez/cli/session.hpp"
#include "beez/core/config/paths/bridge_paths.hpp"
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

class FallbackTestEnv
{
  public:
    FallbackTestEnv()
        : root_(beez::core::systemTempDirectory() / "beez_integration_fallback_env"),
          xdgEnv_("XDG_CONFIG_HOME", (root_ / "xdg").c_str()), cleanup_(root_)
    {
        std::filesystem::create_directories(configDir());
    }

    [[nodiscard]] std::filesystem::path configDir() const { return root_ / "xdg" / "beez"; }
    [[nodiscard]] std::filesystem::path globalDir() const { return configDir() / "global"; }
    [[nodiscard]] std::filesystem::path bridgesDir() const { return configDir() / "bridges"; }

    void writeGlobalBuildLua(const std::string& content) const
    {
        std::filesystem::create_directories(globalDir());
        std::ofstream stream(globalDir() / "build.lua");
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

TEST(SessionFallbackIntegrationTest, UsesGlobalBuildLuaWhenNoLocalAndNoBridge)
{
    const FallbackTestEnv Env;
    Env.writeGlobalBuildLua("task(\"global-task\", \"true\")\n");

    const beez::test::TempProject Project;
    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value())
        << "loadBuildScript should succeed via global fallback";
    EXPECT_EQ(project.context.buildScriptPath(), Env.globalDir() / "build.lua");
}

TEST(SessionFallbackIntegrationTest, LocalBuildLuaWinsOverBridgeAndGlobal)
{
    const FallbackTestEnv Env;
    Env.writeGlobalBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"local-task\", \"true\")\n");

    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());
    EXPECT_EQ(project.context.buildScriptPath(), Project.path() / "build.lua");
}

TEST(SessionFallbackIntegrationTest, BridgeWinsOverGlobal)
{
    const FallbackTestEnv Env;
    Env.writeGlobalBuildLua("task(\"noop\", \"true\")\n");

    const beez::test::TempProject Project;
    Project.writeBuildLua("task(\"bridge-task\", \"true\")\n");
    ASSERT_EQ(beez::core::createBridgeLink(Project.path() / "build.lua", Project.path()).bridgeDir,
              beez::core::bridgeDirectory() / beez::core::hashPath(
                                                            std::filesystem::weakly_canonical(Project.path())));

    std::error_code errorCode;
    std::filesystem::remove(Project.path() / "build.lua", errorCode);

    auto project = makeLoadedProject(Project.path());

    ASSERT_FALSE(beez::cli::loadBuildScript(project, true).has_value());
    EXPECT_EQ(project.context.buildScriptPath().parent_path().parent_path(), Env.bridgesDir());
}

TEST(SessionFallbackIntegrationTest, FailsWhenNoScriptAnywhere)
{
    const FallbackTestEnv Env;
    const beez::test::TempProject Project;

    auto project = makeLoadedProject(Project.path());

    const auto Error = beez::cli::loadBuildScript(project, true);
    ASSERT_TRUE(Error.has_value());
    EXPECT_NE(*Error, 0);
}
