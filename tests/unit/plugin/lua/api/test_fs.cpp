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

TEST(LuaFsApiTest, JoinBuildsPlatformIndependentPath)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("join", "echo " .. beez.fs.join("my", "path", "to", "file.txt"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "join");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo my/path/to/file.txt");
}

TEST(LuaFsApiTest, ExistsReturnsTrueForExistingFile)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "present.txt", "hello");

    Project.writeBuildLua(R"(
task("check", "echo " .. tostring(beez.fs.exists("present.txt")))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaFsApiTest, ExistsReturnsFalseForMissingFile)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("check", "echo " .. tostring(beez.fs.exists("missing.txt")))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo false");
}

TEST(LuaFsApiTest, GlobFindsMatchingFiles)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "alpha.cpp", "");
    writeFile(Project.path() / "src" / "beta.cpp", "");
    writeFile(Project.path() / "src" / "readme.txt", "");

    Project.writeBuildLua(R"(
local files = beez.fs.glob("src/*.cpp")
task("glob", "echo " .. files[1] .. "," .. files[2])
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "glob");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo src/alpha.cpp,src/beta.cpp");
}

TEST(LuaFsApiTest, CopyCreatesDestinationFile)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "source.txt", "payload");

    Project.writeBuildLua(R"(
beez.fs.copy("source.txt", "copied.txt", true)
task("check", "echo " .. tostring(beez.fs.exists("copied.txt")))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    EXPECT_TRUE(std::filesystem::exists(Project.path() / "copied.txt"));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaFsApiTest, CopyWithoutOverwriteFailsToLoadWhenDestinationExists)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "source.txt", "payload");
    writeFile(Project.path() / "target.txt", "existing");

    Project.writeBuildLua(R"(
beez.fs.copy("source.txt", "target.txt")
task("noop", "echo done")
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
}

TEST(LuaFsApiTest, RemoveDeletesFile)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "delete-me.txt", "payload");

    Project.writeBuildLua(R"(
beez.fs.remove("delete-me.txt")
task("check", "echo " .. tostring(beez.fs.exists("delete-me.txt")))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    EXPECT_FALSE(std::filesystem::exists(Project.path() / "delete-me.txt"));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo false");
}

TEST(LuaFsApiTest, FsApiWorksInsideStepCallback)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "in-step.txt", "payload");

    Project.writeBuildLua(R"(
step({
    name = "fs-step",
    phase = "test",
    scope = "code",
    run = function()
        if not beez.fs.exists("in-step.txt") then
            error("expected file to exist")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("fs-step").has_value());
}
