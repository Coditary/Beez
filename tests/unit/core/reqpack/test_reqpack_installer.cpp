#include "beez/core/reqpack/installer.hpp"
#include "beez/core/reqpack/types.hpp"
#include "beez/core/runtime/context.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace
{

beez::core::ReqPackManifest sampleManifest()
{
    beez::core::ReqPackManifest manifest;
    manifest.plugins.emplace("npm",
                             std::vector<beez::core::ReqPackPackage> {
                                 {.name = "eslint", .version = "3.2.1"},
                             });
    return manifest;
}

constexpr const char* SuccessRqpJson = R"({
  "ok": true,
  "dryRun": false,
  "npm": [
    {"name": "eslint", "version": "3.2.1", "status": "installed"}
  ]
})";

constexpr const char* FailureRqpJson = R"({
  "ok": false,
  "dryRun": false,
  "npm": [
    {"name": "eslint", "version": "3.2.1", "status": "failed", "errorMessage": "network timeout"}
  ]
})";

}  // namespace

TEST(ReqPackInstallerTest, SkipsEmptyManifest)
{
    const beez::core::Context Context(std::filesystem::current_path());
    const auto Result = beez::core::installReqPackDependencies({}, Context);
    EXPECT_TRUE(Result.skipped);
    EXPECT_TRUE(Result.success);
}

TEST(ReqPackInstallerTest, DryRunReturnsInstallCommand)
{
    const beez::core::Context Context(std::filesystem::current_path());
    const auto Result = beez::core::installReqPackDependencies(
        sampleManifest(), Context, {.dryRun = true, .forceInstall = true});
    ASSERT_FALSE(Result.skipped);
    EXPECT_TRUE(Result.success);
    EXPECT_NE(Result.message.find("rqp install npm:eslint@3.2.1 --dry-run --json"),
              std::string::npos);
}

TEST(ReqPackInstallerTest, UsesExecuteCallbackAndParsesRqpJson)
{
    const beez::core::Context Context(std::filesystem::current_path());
    std::string capturedCommand;
    const auto Result = beez::core::installReqPackDependencies(
        sampleManifest(),
        Context,
        {.forceInstall = true},
        [&capturedCommand](const std::string& command) -> beez::core::RqpCommandResult
        {
            capturedCommand = command;
            return {.exitCode = 0, .output = SuccessRqpJson};
        });
    EXPECT_TRUE(Result.success);
    EXPECT_EQ(capturedCommand, "rqp install npm:eslint@3.2.1 --json");
    ASSERT_TRUE(Result.response.plugins.contains("npm"));
    EXPECT_EQ(Result.response.plugins.at("npm").at(0).name, "eslint");
}

TEST(ReqPackInstallerTest, ReportsPackageFailureFromRqpJson)
{
    const beez::core::Context Context(std::filesystem::current_path());
    const auto Result = beez::core::installReqPackDependencies(
        sampleManifest(),
        Context,
        {.forceInstall = true},
        [](const std::string& /*command*/) -> beez::core::RqpCommandResult
        { return {.exitCode = 1, .output = FailureRqpJson}; });
    EXPECT_FALSE(Result.success);
    EXPECT_NE(Result.message.find("npm:eslint@3.2.1: network timeout"), std::string::npos);
}

TEST(ReqPackInstallerTest, PropagatesExecuteFailureWithoutJson)
{
    const beez::core::Context Context(std::filesystem::current_path());
    const auto Result = beez::core::installReqPackDependencies(
        sampleManifest(),
        Context,
        {.forceInstall = true},
        [](const std::string& /*command*/) -> beez::core::RqpCommandResult
        { return {.exitCode = 17}; });
    EXPECT_FALSE(Result.success);
    EXPECT_NE(Result.message.find("exit code 17"), std::string::npos);
}

TEST(ReqPackInstallerTest, SucceedsWhenRqpReturnsPlainText)
{
    const beez::core::Context Context(std::filesystem::current_path());
    const auto Result = beez::core::installReqPackDependencies(
        sampleManifest(),
        Context,
        {.forceInstall = true},
        [](const std::string& /*command*/) -> beez::core::RqpCommandResult
        { return {.exitCode = 0, .output = "installed eslint"}; });
    EXPECT_TRUE(Result.success);
}
