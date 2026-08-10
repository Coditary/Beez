#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/api/crypto/detail/crypto_ops.hpp"
#include "beez/plugin/lua/api/net/detail/http_client.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/mock_http_server.hpp"
#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <fstream>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(LuaNetApiTest, GetReadsLocalFileUrl)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "payload.txt", "beez-net-payload");
    const std::string Url = "file://" + (Project.path() / "payload.txt").string();
    Project.writeBuildLua(R"(
local response = beez.net.get(")" +
                          Url + R"(")
local ok = response.ok and response.body == "beez-net-payload"
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaNetApiTest, DownloadCopiesLocalFile)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "source.bin", "download-me");
    const std::string Url = "file://" + (Project.path() / "source.bin").string();
    Project.writeBuildLua(R"(
local result = beez.net.download(")" +
                          Url + R"(", "copied.bin")
local ok = result.bytes == 11 and beez.fs.exists("copied.bin")
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaNetApiTest, RestMethodsUseHeaderTable)
{
    beez::test::MockHttpServer server;
    server.setFixedResponse(200, "posted");

    const beez::test::TempProject Project;
    writeFile(Project.path() / "upload.txt", "upload-body");
    const std::string Script = R"(
local base = ")" + server.baseUrl() +
                               R"("
local headers = {
    ["X-Test"] = "beez",
    Authorization = "token",
}

local get = beez.net.get(base .. "/get", headers)
local post = beez.net.post(base .. "/post", "hello", headers)
local put = beez.net.put(base .. "/put", "updated", headers)
local deleted = beez.net.delete(base .. "/delete", headers)
local custom = beez.net.request("PATCH", base .. "/patch", {
    body = "patch-body",
    headers = headers,
})
local uploaded = beez.net.upload(base .. "/upload", "upload.txt", headers)

local ok = get.ok and post.ok and put.ok and deleted.ok and custom.ok and uploaded.ok
task("check", "echo " .. tostring(ok))
)";
    Project.writeBuildLua(Script);

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
    EXPECT_EQ(server.lastMethod(), "POST");
}

TEST(LuaNetApiTest, DownloadAndVerifyChecksHash)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "hash-source.txt", "verify-me");
    const std::string Hash =
        beez::plugin::lua::crypto_detail::hashFile(Project.path() / "hash-source.txt", "sha256");
    const std::string Url = "file://" + (Project.path() / "hash-source.txt").string();

    Project.writeBuildLua(R"(
local result = beez.net.download_and_verify(")" +
                          Url + R"(", "verified.txt", "sha256", ")" + Hash + R"(")
local ok = result.verified == true
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaNetApiTest, PingLocalServer)
{
    beez::test::MockHttpServer server;
    server.setFixedResponse(204, "");

    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local result = beez.net.ping(")" +
                          server.baseUrl() + R"(")
local ok = result.ok and result.status == 204
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(NetHttpClientTest, PingMeasuresLocalServer)
{
    beez::test::MockHttpServer server;
    server.setFixedResponse(200, "pong");

    const auto Result =
        beez::plugin::lua::net_detail::HttpClient::instance().ping(server.baseUrl(), 5);
    EXPECT_TRUE(Result.reachable);
    EXPECT_EQ(Result.statusCode, 200);
    EXPECT_GE(Result.milliseconds, 0.0);
}

TEST(LuaNetApiTest, SetProxyAndClear)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "proxy-step",
    phase = "test",
    scope = "code",
    run = function()
        beez.net.set_proxy("http://127.0.0.1:8080")
        beez.net.set_proxy(nil)
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("proxy-step").has_value());
}

TEST(LuaNetApiTest, IsOnlineReturnsBoolean)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local value = beez.net.is_online(1)
local ok = value == true or value == false
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}
