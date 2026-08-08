#include "beez/cli/cli_app.hpp"
#include "beez/cli/parsed_options.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

std::vector<const char*> toArgv(const std::vector<std::string>& args)
{
    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (const auto& argument : args)
    {
        argv.push_back(argument.c_str());
    }
    return argv;
}

}  // namespace

TEST(CliAppTest, HelpContainsBannerAndUsage)
{
    const std::string Help = beez::cli::CliApp::helpText();
    EXPECT_NE(Help.find("Beez - Build Everything Easy (0.1.0)"), std::string::npos);
    EXPECT_NE(Help.find("Usage: beez [target] [core-options] [-- user-options]"),
              std::string::npos);
    EXPECT_NE(Help.find("-h, --help"), std::string::npos);
    EXPECT_NE(Help.find("-v, --version"), std::string::npos);
    EXPECT_NE(Help.find("--verbose"), std::string::npos);
    EXPECT_NE(Help.find("--dry-run"), std::string::npos);
    EXPECT_NE(Help.find("--threads"), std::string::npos);
    EXPECT_NE(Help.find("--list TEXT"), std::string::npos);
}

TEST(CliAppTest, VersionContainsBeezAndLua)
{
    const std::string Version = beez::cli::CliApp::versionText();
    EXPECT_NE(Version.find("Beez 0.1.0"), std::string::npos);
    EXPECT_NE(Version.find("Lua 5.4"), std::string::npos);
}

TEST(CliAppTest, HelpFlagRequestsHelp)
{
    const std::vector<std::string> Args = {"beez", "--help"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Help);
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(CliAppTest, VersionFlagRequestsVersion)
{
    const std::vector<std::string> Args = {"beez", "-v"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Version);
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(CliAppTest, ParsesTargetAndFlags)
{
    const std::vector<std::string> Args = {"beez", "build", "--verbose", "--dry-run"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.target, "build");
    EXPECT_TRUE(Result.options.verbose);
    EXPECT_TRUE(Result.options.dryRun);
}

TEST(CliAppTest, ParsesListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "tasks"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.listKind.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.listKind, "tasks");
}

TEST(CliAppTest, ParsesPhasesListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "phases"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.listKind.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.listKind, "phases");
}

TEST(CliAppTest, ParsesUserOptionsAfterSeparator)
{
    const std::vector<std::string> Args = {"beez", "build", "--", "-j8", "release"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_EQ(Result.options.userOptions.size(), 2U);
    EXPECT_EQ(Result.options.userOptions[0], "-j8");
    EXPECT_EQ(Result.options.userOptions[1], "release");
}

TEST(CliAppTest, MissingTargetShowsHelpWithError)
{
    const std::vector<std::string> Args = {"beez"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Help);
    EXPECT_EQ(Result.exitCode, 1);
}

TEST(CliAppTest, RejectsUnknownListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "unknown"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliAppTest, NoCacheDisablesCaching)
{
    const std::vector<std::string> Args = {"beez", "build", "--no-cache"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_FALSE(Result.options.enableCache);
}

TEST(CliAppTest, ParsesThreadsFlag)
{
    const std::vector<std::string> Args = {"beez", "build", "--threads", "4"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.maxThreads.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.maxThreads.value(), 4U);
}

TEST(CliAppTest, ParsesJobsAlias)
{
    const std::vector<std::string> Args = {"beez", "build", "-j", "2"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.maxThreads.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.maxThreads.value(), 2U);
}

TEST(CliAppTest, RejectsZeroThreads)
{
    const std::vector<std::string> Args = {"beez", "build", "--threads", "0"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliAppTest, ParsesConfigOptionsFlag)
{
    const std::vector<std::string> Args = {"beez", "--config-options", "cache.hash"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.configOptions);
    EXPECT_EQ(Result.options.configOptionsPath, "cache.hash");
}

TEST(CliAppTest, ParsesConfigOptionsWithoutPath)
{
    const std::vector<std::string> Args = {"beez", "--config-options"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.configOptions);
    EXPECT_TRUE(Result.options.configOptionsPath.empty());
}

TEST(CliAppTest, ParsesShowConfigFlag)
{
    const std::vector<std::string> Args = {"beez", "--show-config"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.showConfig);
}

TEST(CliAppTest, ParsesCleanCacheFlag)
{
    const std::vector<std::string> Args = {"beez", "--clean-cache"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.cleanCache);
}

TEST(CliAppTest, ParsesInstallCompletionFlag)
{
    const std::vector<std::string> Args = {"beez", "--install-completion"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliApp::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.installCompletion);
}
