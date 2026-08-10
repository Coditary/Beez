#include "beez/core/util/temp_directory.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>

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

// NOLINTEND(misc-include-cleaner)

}  // namespace

TEST(TempDirectoryTest, SystemTempDirectoryIsAbsolute)
{
    const auto Temp = beez::core::systemTempDirectory();
    EXPECT_FALSE(Temp.empty());
    EXPECT_TRUE(Temp.is_absolute());
}

TEST(TempDirectoryTest, IgnoresRelativeTmpdirEnvironmentValue)
{
    const ScopedEnv Tmpdir("TMPDIR", "tmp");

    const auto Temp = beez::core::systemTempDirectory();
    EXPECT_TRUE(Temp.is_absolute());
    EXPECT_NE(Temp, std::filesystem::path("tmp"));
}
