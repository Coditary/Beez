#include "helpers/process_runner.hpp"
#include "helpers/temp_project.hpp"

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

class LinkTestEnv
{
  public:
    LinkTestEnv()
        : tempRoot_(beez::core::systemTempDirectory() / "beez_link_integration_test"),
          xdgEnv_("XDG_CONFIG_HOME", (tempRoot_ / "xdg").c_str()), cleanup_(tempRoot_)
    {
        std::filesystem::create_directories(tempRoot_ / "xdg" / "beez");
    }

    [[nodiscard]] const std::filesystem::path& tempRoot() const { return tempRoot_; }
    [[nodiscard]] std::filesystem::path bridgesDir() const
    {
        return tempRoot_ / "xdg" / "beez" / "bridges";
    }
    [[nodiscard]] std::filesystem::path indexPath() const { return bridgesDir() / "index.json"; }

  private:
    std::filesystem::path tempRoot_;
    ScopedEnv xdgEnv_;
    ScopedTempTree cleanup_;
};

}  // namespace

TEST(CliLinkIntegrationTest, LinkWithoutPathCopiesBuildLua)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("echo-task", "echo hello-from-link")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--link"}, {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Linked"), std::string::npos);
    EXPECT_NE(Result.output.find("Bridge:"), std::string::npos);

    EXPECT_TRUE(std::filesystem::exists(env.indexPath()));
}

TEST(CliLinkIntegrationTest, LinkWithPathCopiesCustomBuildLua)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;

    const auto CustomDir = Project.path() / "custom";
    std::filesystem::create_directories(CustomDir);
    {
        std::ofstream stream(CustomDir / "my-build.lua");
        stream << R"(
task("custom-task", "echo custom-link")
)";
    }

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--link", (CustomDir / "my-build.lua").string()},
                            {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    EXPECT_EQ(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Linked"), std::string::npos);
    EXPECT_NE(Result.output.find("Bridge:"), std::string::npos);

    EXPECT_TRUE(std::filesystem::exists(env.indexPath()));
}

TEST(CliLinkIntegrationTest, LinkWhenAlreadyLinkedShowsWarning)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");

    const std::string XdgVar = "XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string();

    const auto First = beez::test::runBeez(Project.path(), {"--link"}, {XdgVar});
    EXPECT_EQ(First.exitCode, 0);
    EXPECT_NE(First.output.find("Linked"), std::string::npos);

    const auto Second = beez::test::runBeez(Project.path(), {"--link"}, {XdgVar});
    EXPECT_EQ(Second.exitCode, 0);
    EXPECT_NE(Second.output.find("Warning: bridge already exists"), std::string::npos);
    EXPECT_NE(Second.output.find("Bridge:"), std::string::npos);
}

TEST(CliLinkIntegrationTest, LinkWithMissingBuildLuaShowsError)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--link"}, {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Error:"), std::string::npos);
}

TEST(CliLinkIntegrationTest, LinkWithMissingCustomPathShowsError)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");

    const beez::test::ProcessResult Result =
        beez::test::runBeez(Project.path(), {"--link", "/nonexistent/build.lua"},
                            {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    EXPECT_NE(Result.exitCode, 0);
    EXPECT_NE(Result.output.find("Error:"), std::string::npos);
}

TEST(CliLinkIntegrationTest, BridgeDirectoryContainsHash)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");

    beez::test::runBeez(Project.path(), {"--link"},
                        {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    const auto BridgesDir = env.bridgesDir();
    ASSERT_TRUE(std::filesystem::exists(BridgesDir));

    bool foundHashDir = false;
    for (const auto& entry : std::filesystem::directory_iterator(BridgesDir))
    {
        if (entry.is_directory())
        {
            const auto BuildLua = entry.path() / "build.lua";
            if (std::filesystem::exists(BuildLua))
            {
                foundHashDir = true;

                std::ifstream file(BuildLua);
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                EXPECT_NE(content.find("task(\"noop\""), std::string::npos);
            }
        }
    }
    EXPECT_TRUE(foundHashDir);
}

TEST(CliLinkIntegrationTest, IndexJsonContainsProjectPath)
{
    LinkTestEnv env;
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("noop", "true")
)");

    beez::test::runBeez(Project.path(), {"--link"},
                        {"XDG_CONFIG_HOME=" + (env.tempRoot() / "xdg").string()});

    ASSERT_TRUE(std::filesystem::exists(env.indexPath()));
    std::ifstream file(env.indexPath());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    const auto Canonical = std::filesystem::weakly_canonical(Project.path()).string();
    EXPECT_NE(content.find(Canonical), std::string::npos);
    EXPECT_NE(content.find("\"hash\""), std::string::npos);
}
