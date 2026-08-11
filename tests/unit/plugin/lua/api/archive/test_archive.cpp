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

TEST(LuaArchiveApiTest, CompressExtractAndListZip)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "src" / "hello.txt", "hello-archive");
    writeFile(Project.path() / "src" / "nested" / "note.txt", "nested-file");

    Project.writeBuildLua(R"(
beez.archive.compress("src", "bundle.zip")
local entries = beez.archive.list("bundle.zip")
local found_hello = false
for _, entry in ipairs(entries) do
    if entry.path == "hello.txt" then
        found_hello = true
    end
end

beez.archive.extract("bundle.zip", "out")
local extracted = beez.fs.exists("out/nested/note.txt")
local text = beez.archive.read_text("bundle.zip", "hello.txt")
beez.archive.extract_file("bundle.zip", "nested/note.txt", "single.txt")

local ok = extracted
    and text == "hello-archive"
    and beez.fs.exists("single.txt")
    and found_hello
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaArchiveApiTest, CompressTarGzWithExplicitFormat)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "payload.txt", "tar-gz-content");

    Project.writeBuildLua(R"(
beez.archive.compress("payload.txt", "payload.tar.gz", { format = "tar.gz" })
local text = beez.archive.read_text("payload.tar.gz", "payload.txt")
task("check", "echo " .. tostring(text == "tar-gz-content"))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaArchiveApiTest, ListReportsDirectoryEntries)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "tree" / "a.txt", "a");

    Project.writeBuildLua(R"(
beez.archive.compress("tree", "tree.zip")
local count = 0
for _, entry in ipairs(beez.archive.list("tree.zip")) do
    count = count + 1
end
task("check", "echo " .. tostring(count >= 1))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}
