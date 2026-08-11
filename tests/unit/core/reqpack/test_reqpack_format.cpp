#include "beez/core/reqpack/format.hpp"
#include "beez/core/reqpack/types.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
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
                                 {.name = "vitest", .version = "1.2.3"},
                             });
    manifest.plugins.emplace("sys",
                             std::vector<beez::core::ReqPackPackage> {
                                 {.name = "make"},
                                 {.name = "cmake"},
                             });
    return manifest;
}

constexpr const char* LegacyRqpJson =
    R"({"npm":[{"name":"eslint","version":"3.2.1"},{"name":"vitest","version":"1.2.3"}],"sys":[{"name":"make"}]})";

constexpr const char* EnrichedFailureRqpJson = R"({
  "ok": false,
  "dryRun": false,
  "npm": [
    {"name": "eslint", "version": "9.0.0", "status": "installed"},
    {"name": "vitest", "version": "", "status": "failed", "errorMessage": "plugin action failed"}
  ]
})";

}  // namespace

TEST(ReqPackFormatTest, FormatsInstallArgWithVersion)
{
    const beez::core::ReqPackPackage Package {.name = "eslint", .version = "3.2.1"};
    EXPECT_EQ(beez::core::formatInstallArg("npm", Package), "npm:eslint@3.2.1");
}

TEST(ReqPackFormatTest, FormatsInstallArgWithoutVersion)
{
    const beez::core::ReqPackPackage Package {.name = "make"};
    EXPECT_EQ(beez::core::formatInstallArg("sys", Package), "sys:make");
}

TEST(ReqPackFormatTest, BuildsInstallArgsAcrossPlugins)
{
    const auto Args = beez::core::buildInstallArgs(sampleManifest());
    ASSERT_EQ(Args.size(), 4U);
    EXPECT_EQ(Args.at(0), "npm:eslint@3.2.1");
    EXPECT_EQ(Args.at(1), "npm:vitest@1.2.3");
    EXPECT_EQ(Args.at(2), "sys:make");
    EXPECT_EQ(Args.at(3), "sys:cmake");
}

TEST(ReqPackFormatTest, BuildsInstallCommandWithJsonFlag)
{
    const auto Command = beez::core::buildInstallCommand({"npm:eslint@3.2.1", "sys:make"});
    EXPECT_EQ(Command, "rqp --json install npm:eslint@3.2.1 sys:make");
}

TEST(ReqPackFormatTest, BuildsDryRunInstallCommand)
{
    const auto Command = beez::core::buildInstallCommand({"npm:eslint@3.2.1"}, true);
    EXPECT_EQ(Command, "rqp --json install npm:eslint@3.2.1 --dry-run");
}

TEST(ReqPackFormatTest, ParsesLegacyRqpJsonResponse)
{
    const auto Response = beez::core::parseRqpJsonResponse(LegacyRqpJson);
    EXPECT_TRUE(Response.succeeded());
    ASSERT_EQ(Response.plugins.size(), 2U);
    ASSERT_TRUE(Response.plugins.contains("npm"));
    const auto& npmPackages = Response.plugins.at("npm");
    EXPECT_EQ(npmPackages.at(0).name, "eslint");
    ASSERT_TRUE(npmPackages.at(0).version.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*npmPackages.at(0).version, "3.2.1");
    EXPECT_FALSE(npmPackages.at(0).status.has_value());
}

TEST(ReqPackFormatTest, ParsesEnrichedRqpJsonResponse)
{
    const auto Response = beez::core::parseRqpJsonResponse(EnrichedFailureRqpJson);
    ASSERT_TRUE(Response.ok.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_FALSE(*Response.ok);
    EXPECT_FALSE(Response.dryRun);
    ASSERT_TRUE(Response.plugins.contains("npm"));
    const auto& npmPackages = Response.plugins.at("npm");
    ASSERT_EQ(npmPackages.size(), 2U);
    EXPECT_EQ(npmPackages.at(0).status.value_or(""), "installed");
    EXPECT_EQ(npmPackages.at(1).status.value_or(""), "failed");
    ASSERT_TRUE(npmPackages.at(1).errorMessage.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*npmPackages.at(1).errorMessage, "plugin action failed");
    EXPECT_FALSE(Response.succeeded());
}

TEST(ReqPackFormatTest, FormatsFailedPackageErrors)
{
    const auto Response = beez::core::parseRqpJsonResponse(EnrichedFailureRqpJson);
    const auto Message = beez::core::formatRqpInstallErrors(Response);
    EXPECT_NE(Message.find("npm:vitest: plugin action failed"), std::string::npos);
    EXPECT_EQ(Message.find("eslint"), std::string::npos);
}

TEST(ReqPackFormatTest, SuccessfulPackagesOmitsFailures)
{
    const auto Response = beez::core::parseRqpJsonResponse(EnrichedFailureRqpJson);
    const auto Packages = Response.successfulPackages();
    ASSERT_TRUE(Packages.plugins.contains("npm"));
    ASSERT_EQ(Packages.plugins.at("npm").size(), 1U);
    EXPECT_EQ(Packages.plugins.at("npm").at(0).name, "eslint");
}

TEST(ReqPackFormatTest, RejectsInvalidRqpJson)
{
    EXPECT_THROW(beez::core::parseRqpJsonResponse("not-json"), std::runtime_error);
}
