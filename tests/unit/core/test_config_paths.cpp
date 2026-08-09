#include "beez/core/config_paths.hpp"
#include "beez/core/util/temp_directory.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
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

  private:
    std::filesystem::path path_;
};

// NOLINTEND(misc-include-cleaner)

}  // namespace

TEST(ConfigPathsTest, UsesXdgConfigHomeWhenSet)
{
    const auto Root = beez::core::systemTempDirectory() / "beez_config_paths_xdg_test";
    const ScopedTempTree Cleanup(Root);
    std::filesystem::create_directories(Root);
    const ScopedEnv Xdg("XDG_CONFIG_HOME", Root.c_str());

    EXPECT_EQ(beez::core::beezConfigDirectory(), Root / "beez");
    EXPECT_EQ(beez::core::globalBeezConfigPath(), Root / "beez" / "config.lua");
}

TEST(ConfigPathsTest, FallsBackToHomeDotConfigWhenXdgUnset)
{
    const auto HomeRoot = beez::core::systemTempDirectory() / "beez_config_paths_home_test";
    const ScopedTempTree Cleanup(HomeRoot);
    std::filesystem::create_directories(HomeRoot);
    const ScopedEnv Home("HOME", HomeRoot.c_str());
    const ScopedEnv XdgUnset("XDG_CONFIG_HOME", "");

    EXPECT_EQ(beez::core::beezConfigDirectory(), HomeRoot / ".config" / "beez");
    EXPECT_EQ(beez::core::globalBeezConfigPath(), HomeRoot / ".config" / "beez" / "config.lua");
}

TEST(ConfigPathsTest, ReturnsEmptyWhenHomeUnset)
{
    const ScopedEnv HomeUnset("HOME", "");
    const ScopedEnv XdgUnset("XDG_CONFIG_HOME", "");

    EXPECT_TRUE(beez::core::beezConfigDirectory().empty());
    EXPECT_TRUE(beez::core::globalBeezConfigPath().empty());
}
