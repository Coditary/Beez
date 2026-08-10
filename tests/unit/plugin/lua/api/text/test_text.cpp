#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(LuaTextApiTest, StringHelpers)
{
    const beez::test::TempProject Project;

    Project.writeBuildLua(R"(
local ok = beez.text.contains("hello world", "world")
    and beez.text.starts_with("hello world", "hello")
    and beez.text.ends_with("hello world", "world")
    and beez.text.to_lowercase("AbC") == "abc"
    and beez.text.to_uppercase("AbC") == "ABC"
    and beez.text.to_case("hello WORLD") == "Hello World"
    and beez.text.replace("one two one", "one", "x") == "x two one"
    and beez.text.replace_all("one two one", "one", "x") == "x two x"
    and beez.text.trim("  spaced  ") == "spaced"
    and beez.text.join({ "a", "b", "c" }, "-") == "a-b-c"
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaTextApiTest, SplitRegexAndTemplateSkeleton)
{
    const beez::test::TempProject Project;

    Project.writeBuildLua(R"(
local parts = beez.text.split("a,b,c", ",")
local split_ok = #parts == 3 and parts[1] == "a" and parts[3] == "c"
local regex_ok = beez.text.regex_match("abc123", "^abc%d+$")
    and beez.text.regex_replace("foo bar foo", "foo", "baz") == "baz bar baz"
local template_ok = beez.text.template("Hello {{name}}", { name = "Beez" }) == "Hello Beez"
    and beez.text.template("{{ if ready }}yes{{ else }}no{{ endif }}", { ready = true }) == "yes"
    and beez.text.template("{{ items[1] }}-{{ user.name }}", { items = { "a", "b" }, user = { name = "Ada" } }) == "a-Ada"
task("check", "echo " .. tostring(split_ok and regex_ok and template_ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaTextApiTest, LineDiff)
{
    const beez::test::TempProject Project;

    Project.writeBuildLua(R"(
local diff = beez.text.diff("alpha\nbeta\n", "alpha\ngamma\n")
local insert_ok = false
local delete_ok = false
for _, chunk in ipairs(diff) do
    if chunk.op == "insert" and chunk.text == "gamma" then
        insert_ok = true
    end
    if chunk.op == "delete" and chunk.text == "beta" then
        delete_ok = true
    end
end
task("check", "echo " .. tostring(insert_ok and delete_ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}
