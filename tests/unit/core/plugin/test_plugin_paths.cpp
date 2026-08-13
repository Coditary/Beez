#include "beez/core/plugin/paths.hpp"
#include "beez/core/util/temp_directory.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace
{

// NOLINTBEGIN(misc-include-cleaner) -- setenv/unsetenv provided by cstdlib on Linux
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
            else
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
                unsetenv(name_.c_str());
            }
        }
        else if (hadValue_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-command-processor,cert-env33-c)
            setenv(name_.c_str(), saved_.c_str(), 1);
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
    explicit ScopedTempTree(const std::filesystem::path& path) : path_(path) {}

    ~ScopedTempTree()
    {
        std::error_code errorCode;
        std::filesystem::remove_all(path_, errorCode);
    }

    ScopedTempTree(const ScopedTempTree&) = delete;
    ScopedTempTree& operator=(const ScopedTempTree&) = delete;

  private:
    std::filesystem::path path_;
};

// NOLINTEND(misc-include-cleaner)

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(PluginPathsTest, UsesXdgCacheHomeWhenSet)
{
    const auto Root = beez::core::systemTempDirectory() / "beez_plugin_paths_xdg_test";
    const ScopedTempTree Cleanup(Root);
    std::filesystem::create_directories(Root);
    const ScopedEnv Xdg("XDG_CACHE_HOME", Root.c_str());

    EXPECT_EQ(beez::core::beezCacheDirectory(), Root / "beez");
    EXPECT_EQ(beez::core::beezPluginRoot(), Root / "beez" / "plugins");
}

TEST(PluginPathsTest, FallsBackToHomeDotCacheWhenXdgUnset)
{
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_plugin_paths_home_test";
    const ScopedTempTree Cleanup(HomeRoot);
    std::filesystem::create_directories(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CACHE_HOME", "");

    EXPECT_EQ(beez::core::beezCacheDirectory(), HomeRoot / ".cache" / "beez");
    EXPECT_EQ(beez::core::beezPluginRoot(), HomeRoot / ".cache" / "beez" / "plugins");
}

TEST(PluginPathsTest, FindsPluginScriptAcrossOrganizations)
{
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_plugin_paths_find_test";
    const ScopedTempTree Cleanup(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CACHE_HOME", "");

    const auto PluginPath = HomeRoot / ".cache" / "beez" / "plugins" / "coditary" / "my_plugin" /
                            "1.0.0" / "beez_plugin.lua";
    writeFile(PluginPath, "plugin('my_plugin', { version = '1.0.0' })");

    const auto Found = beez::core::findPluginScript("my_plugin", "1.0.0");
    ASSERT_TRUE(Found.has_value());
    EXPECT_EQ(*Found, PluginPath);
}

TEST(PluginPathsTest, ReturnsEmptyWhenPluginMissing)
{
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_plugin_paths_missing_test";
    const ScopedTempTree Cleanup(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CACHE_HOME", "");

    const auto Found = beez::core::findPluginScript("missing", "1.0.0");
    EXPECT_FALSE(Found.has_value());
}

TEST(PluginPathsTest, FindsPluginScriptForQualifiedOrganization)
{
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_plugin_paths_qualified_test";
    const ScopedTempTree Cleanup(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CACHE_HOME", "");

    const auto PluginPath = HomeRoot / ".cache" / "beez" / "plugins" / "coditary" / "hello" /
                            "1.0.0" / "beez_plugin.lua";
    writeFile(PluginPath, "plugin('hello', { version = '1.0.0' })");

    const auto Found = beez::core::findPluginScript("coditary", "hello", "1.0.0");
    ASSERT_TRUE(Found.has_value());
    EXPECT_EQ(*Found, PluginPath);
}
