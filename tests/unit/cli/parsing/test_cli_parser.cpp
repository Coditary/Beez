#include "beez/cli/completion/install_completion.hpp"
#include "beez/cli/parsing/cli_parser.hpp"
#include "beez/cli/parsing/help_text.hpp"
#include "beez/cli/parsing/parsed_options.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
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

TEST(CliParserTest, HelpContainsBannerAndUsage)
{
    const std::string Help = beez::cli::helpText();
    EXPECT_NE(Help.find("Beez - Build Everything Easy (0.1.0)"), std::string::npos);
    EXPECT_NE(Help.find("Usage: beez [target] [core-options] [-- user-options]"),
              std::string::npos);
    EXPECT_NE(Help.find("-h, --help"), std::string::npos);
    EXPECT_NE(Help.find("-v, --version"), std::string::npos);
    EXPECT_NE(Help.find("--verbose"), std::string::npos);
    EXPECT_NE(Help.find("--silent"), std::string::npos);
    EXPECT_NE(Help.find("--error"), std::string::npos);
    EXPECT_NE(Help.find("--dry-run"), std::string::npos);
    EXPECT_NE(Help.find("--threads"), std::string::npos);
    EXPECT_NE(Help.find("--list TEXT"), std::string::npos);
}

TEST(CliParserTest, VersionContainsBeezAndLua)
{
    const std::string Version = beez::cli::versionText();
    EXPECT_NE(Version.find("Beez 0.1.0"), std::string::npos);
    EXPECT_NE(Version.find("Lua 5.4"), std::string::npos);
}

TEST(CliParserTest, HelpFlagRequestsHelp)
{
    const std::vector<std::string> Args = {"beez", "--help"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Help);
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(CliParserTest, VersionFlagRequestsVersion)
{
    const std::vector<std::string> Args = {"beez", "-v"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Version);
    EXPECT_EQ(Result.exitCode, 0);
}

TEST(CliParserTest, ParsesSilentAndErrorFlags)
{
    {
        const std::vector<std::string> Args = {"beez", "build", "--silent"};
        const auto Argv = toArgv(Args);
        const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
        ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
        EXPECT_TRUE(Result.options.silent);
        EXPECT_FALSE(Result.options.errorsOnly);
        EXPECT_FALSE(Result.options.verbose);
    }

    {
        const std::vector<std::string> Args = {"beez", "build", "--error"};
        const auto Argv = toArgv(Args);
        const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
        ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
        EXPECT_TRUE(Result.options.errorsOnly);
        EXPECT_FALSE(Result.options.silent);
        EXPECT_FALSE(Result.options.verbose);
    }
}

TEST(CliParserTest, RejectsConflictingOutputFlags)
{
    const std::vector<std::string> Args = {"beez", "build", "--silent", "--verbose"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliParserTest, ParsesTargetAndFlags)
{
    const std::vector<std::string> Args = {"beez", "build", "--verbose", "--dry-run"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.target, "build");
    EXPECT_TRUE(Result.options.verbose);
    EXPECT_TRUE(Result.options.dryRun);
}

TEST(CliParserTest, ParsesListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "tasks"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.listKind.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.listKind, "tasks");
}

TEST(CliParserTest, ParsesPhasesListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "phases"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.listKind.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Result.options.listKind, "phases");
}

TEST(CliParserTest, ParsesUserOptionsAfterSeparator)
{
    const std::vector<std::string> Args = {"beez", "build", "--", "-j8", "release"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_EQ(Result.options.userOptions.size(), 2U);
    EXPECT_EQ(Result.options.userOptions[0], "-j8");
    EXPECT_EQ(Result.options.userOptions[1], "release");
}

TEST(CliParserTest, MissingTargetShowsHelpWithError)
{
    const std::vector<std::string> Args = {"beez"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Help);
    EXPECT_EQ(Result.exitCode, 1);
}

TEST(CliParserTest, RejectsUnknownListKind)
{
    const std::vector<std::string> Args = {"beez", "--list", "unknown"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliParserTest, NoCacheDisablesCaching)
{
    const std::vector<std::string> Args = {"beez", "build", "--no-cache"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_FALSE(Result.options.enableCache);
}

TEST(CliParserTest, ParsesThreadsFlag)
{
    const std::vector<std::string> Args = {"beez", "build", "--threads", "4"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.maxThreads.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.maxThreads.value(), 4U);
}

TEST(CliParserTest, ParsesJobsAlias)
{
    const std::vector<std::string> Args = {"beez", "build", "-j", "2"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.maxThreads.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.maxThreads.value(), 2U);
}

TEST(CliParserTest, RejectsZeroThreads)
{
    const std::vector<std::string> Args = {"beez", "build", "--threads", "0"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliParserTest, ParsesConfigOptionsFlag)
{
    const std::vector<std::string> Args = {"beez", "--config-options", "cache.hash"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.configOptions);
    EXPECT_EQ(Result.options.configOptionsPath, "cache.hash");
}

TEST(CliParserTest, ParsesConfigOptionsWithoutPath)
{
    const std::vector<std::string> Args = {"beez", "--config-options"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.configOptions);
    EXPECT_TRUE(Result.options.configOptionsPath.empty());
}

TEST(CliParserTest, ParsesCompleteConfigOptionsFlag)
{
    const std::vector<std::string> Args = {
        "beez", "--complete-config-options", "performance.cache"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.completeConfigOptions);
    EXPECT_EQ(Result.options.completeConfigOptionsPrefix, "performance.cache");
}

TEST(CliParserTest, ParsesDumpCompletionFlag)
{
    const std::vector<std::string> Args = {"beez", "--dump-completion", "zsh"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.dumpCompletion);
    EXPECT_EQ(Result.options.dumpCompletionShell, "zsh");
}

TEST(CliParserTest, DumpCompletionScriptContainsConfigOptions)
{
    const std::string Content = std::string(beez::cli::dumpCompletionScript("zsh").value_or(""));
    ASSERT_FALSE(Content.empty());
    EXPECT_NE(Content.find("--config-options"), std::string::npos);
    EXPECT_NE(Content.find("--complete-config-options"), std::string::npos);
}

TEST(CliParserTest, ParsesShowConfigFlag)
{
    const std::vector<std::string> Args = {"beez", "--show-config"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.showConfig);
}

TEST(CliParserTest, ParsesCleanCacheFlag)
{
    const std::vector<std::string> Args = {"beez", "--clean-cache"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.cleanCache);
}

TEST(CliParserTest, ParsesUpdateFlag)
{
    const std::vector<std::string> Args = {"beez", "--update"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.updateCache);
}

TEST(CliParserTest, ParsesInstallCompletionFlag)
{
    const std::vector<std::string> Args = {"beez", "--install-completion"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.installCompletion);
}

TEST(CliParserTest, ParsesInstallFlag)
{
    const std::vector<std::string> Args = {"beez", "--install"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.installDependencies);
}

TEST(CliParserTest, ParsesLogFileFlag)
{
    const std::vector<std::string> Args = {"beez", "build", "--log-file", "/tmp/beez-run.log"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.logFile.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.logFile.value(), std::filesystem::path("/tmp/beez-run.log"));
    EXPECT_FALSE(Result.options.noLogFile);
}

TEST(CliParserTest, ParsesNoLogFileFlag)
{
    const std::vector<std::string> Args = {"beez", "build", "--no-log-file"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.noLogFile);
    EXPECT_FALSE(Result.options.logFile.has_value());
}

TEST(CliParserTest, RejectsInvalidDumpCompletionShell)
{
    const std::vector<std::string> Args = {"beez", "--dump-completion", "fish"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliParserTest, RejectsSilentAndErrorTogether)
{
    const std::vector<std::string> Args = {"beez", "build", "--silent", "--error"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
}

TEST(CliParserTest, ParsesStepFlag)
{
    const std::vector<std::string> Args = {"beez", "-s", "compile"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.stepName.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.stepName.value(), "compile");
}

TEST(CliParserTest, ParsesPhaseFlag)
{
    const std::vector<std::string> Args = {"beez", "-p", "generate:code"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.phaseRequest.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.phaseRequest->phase, "generate");
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    ASSERT_EQ(Result.options.phaseRequest->scopes.size(), 1U);
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.phaseRequest->scopes[0], "code");
}

TEST(CliParserTest, ParsesLinkFlagWithoutPath)
{
    const std::vector<std::string> Args = {"beez", "--link"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.linkPath.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_TRUE(Result.options.linkPath->empty());
}

TEST(CliParserTest, ParsesLinkFlagWithPath)
{
    const std::vector<std::string> Args = {"beez", "--link", "/my/custom/build.lua"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.linkPath.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.linkPath.value(), std::filesystem::path("/my/custom/build.lua"));
}

TEST(CliParserTest, LinkFlagSatisfiesRunTarget)
{
    const std::vector<std::string> Args = {"beez", "--link"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    // --link alone should not show help (it's a valid run target)
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
}

TEST(CliParserTest, LinkFlagWithTargetCombined)
{
    const std::vector<std::string> Args = {"beez", "--link", "/path/to/build.lua", "mytask"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.linkPath.has_value());
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(Result.options.target.value(), "mytask");
}

TEST(CliParserTest, ParsesBridgeFlagShortLBeforeTarget)
{
    const std::vector<std::string> Args = {"beez", "-l", "hello"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.fromBridge);
    EXPECT_FALSE(Result.options.fromGlobal);
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(Result.options.target.value(), "hello");
}

TEST(CliParserTest, ParsesGlobalFlagShortGBeforeTarget)
{
    const std::vector<std::string> Args = {"beez", "-g", "hello"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(Result.options.fromGlobal);
    EXPECT_FALSE(Result.options.fromBridge);
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    EXPECT_EQ(Result.options.target.value(), "hello");
}

TEST(CliParserTest, SourceFlagsAcceptedAfterTarget)
{
    const std::vector<std::string> ArgsL = {"beez", "hello", "-l"};
    const auto ArgvL = toArgv(ArgsL);
    const auto ResultL = beez::cli::CliParser::parse(static_cast<int>(ArgvL.size()), ArgvL.data());
    ASSERT_EQ(ResultL.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(ResultL.options.fromBridge);

    const std::vector<std::string> ArgsG = {"beez", "hello", "-g"};
    const auto ArgvG = toArgv(ArgsG);
    const auto ResultG = beez::cli::CliParser::parse(static_cast<int>(ArgvG.size()), ArgvG.data());
    ASSERT_EQ(ResultG.reason, beez::cli::CliExitReason::Continue);
    EXPECT_TRUE(ResultG.options.fromGlobal);
}

TEST(CliParserTest, BridgeAndGlobalFlagsRejectedTogether)
{
    const std::vector<std::string> Args = {"beez", "-l", "-g", "hello"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    EXPECT_EQ(Result.reason, beez::cli::CliExitReason::Error);
    EXPECT_NE(Result.exitCode, 0);
}

TEST(CliParserTest, ParsesProfileFlag)
{
    const std::vector<std::string> Args = {"beez", "build", "--profile", "dev"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.profile.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.profile.value(), "dev");
}

TEST(CliParserTest, ParsesProfileFlagWithTarget)
{
    const std::vector<std::string> Args = {"beez", "--profile", "release", "build"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    ASSERT_TRUE(Result.options.profile.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.profile.value(), "release");
    ASSERT_TRUE(Result.options.target.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(Result.options.target.value(), "build");
}

TEST(CliParserTest, ProfileFlagNotSetByDefault)
{
    const std::vector<std::string> Args = {"beez", "build"};
    const auto Argv = toArgv(Args);
    const auto Result = beez::cli::CliParser::parse(static_cast<int>(Argv.size()), Argv.data());
    ASSERT_EQ(Result.reason, beez::cli::CliExitReason::Continue);
    EXPECT_FALSE(Result.options.profile.has_value());
}

TEST(CliParserTest, HelpContainsProfileOption)
{
    const std::string Help = beez::cli::helpText();
    EXPECT_NE(Help.find("--profile NAME"), std::string::npos);
    EXPECT_NE(Help.find("Profiles:"), std::string::npos);
}

TEST(InstallCompletionTest, DumpsEmbeddedBashAndZshScripts)
{
    const auto BashScript = beez::cli::dumpCompletionScript("bash");
    ASSERT_TRUE(BashScript.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_NE(BashScript->find("beez"), std::string_view::npos);

    const auto ZshScript = beez::cli::dumpCompletionScript("zsh");
    ASSERT_TRUE(ZshScript.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_NE(ZshScript->find("beez"), std::string_view::npos);

    EXPECT_FALSE(beez::cli::dumpCompletionScript("fish").has_value());
}
