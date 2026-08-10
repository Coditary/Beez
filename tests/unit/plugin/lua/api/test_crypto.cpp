#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

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

TEST(LuaCryptoApiTest, ListHashAlgoReturnsSupportedAlgorithms)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local algos = beez.crypto.list_hash_algo()
task("check", "echo " .. algos[1] .. "," .. algos[#algos])
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo sha256,sdbm");
}

TEST(LuaCryptoApiTest, ListEncodeAlgoReturnsSupportedAlgorithms)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local algos = beez.crypto.list_encode_algo()
task("check", "echo " .. algos[1] .. "," .. algos[2])
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo hex,base64");
}

TEST(LuaCryptoApiTest, IsHashRecognizesSupportedAlgorithms)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local checks = {
    beez.crypto.is_hash("sha256"),
    beez.crypto.is_hash("sha512"),
    beez.crypto.is_hash("md5"),
    beez.crypto.is_hash("fnv1a64"),
    not beez.crypto.is_hash("unknown"),
}
task("check", "echo " .. tostring(checks[1] and checks[2] and checks[3] and checks[4] and checks[5]))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaCryptoApiTest, IsEncodingAlgoRecognizesSupportedAlgorithms)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local ok = beez.crypto.is_encoding_algo("hex")
    and beez.crypto.is_encoding_algo("base64")
    and not beez.crypto.is_encoding_algo("sha256")
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaCryptoApiTest, HashStringSha256ProducesKnownDigest)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("hash", "echo " .. beez.crypto.hash_string("hello", "sha256"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "hash");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(
        Found, 0, "echo 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(LuaCryptoApiTest, HashFileDefaultsToSha256)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "payload.txt", "hello");

    Project.writeBuildLua(R"(
task("hash", "echo " .. beez.crypto.hash_file("payload.txt"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "hash");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(
        Found, 0, "echo 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(LuaCryptoApiTest, EncodeDefaultsToHex)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("encode", "echo " .. beez.crypto.encode("ab"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "encode");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo 6162");
}

TEST(LuaCryptoApiTest, EncodeSupportsBase64)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("encode", "echo " .. beez.crypto.encode("hello", "base64"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "encode");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo aGVsbG8=");
}

TEST(LuaCryptoApiTest, EncodeWithKeyProducesHmacSha256)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("encode", "echo " .. beez.crypto.encode("hello", "secret", "sha256"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "encode");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(
        Found, 0, "echo 88aab3ede8d3adf94d26ab90d3bafd4a2083070c3bcce9c014ee04a443847c0b");
}

TEST(LuaCryptoApiTest, UnknownHashAlgorithmFailsToLoad)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("hash", "echo " .. beez.crypto.hash_string("hello", "nope"))
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaCryptoApiTest, CryptoApiWorksInsideStepCallback)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "crypto-step",
    phase = "test",
    scope = "code",
    run = function()
        if beez.crypto.hash_string("hello", "sha256") ~= "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824" then
            error("unexpected sha256 digest")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("crypto-step").has_value());
}
