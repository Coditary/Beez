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
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(LuaDataApiTest, DeserializeAndSerializeJsonString)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local data = beez.data.deserialize_string('{"name":"beez","count":3}', { type = "json" })
local back = beez.data.serialize_string(data, { type = "json" })
local ok = data.name == "beez" and data.count == 3 and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, FileRoundTripUsesExtension)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "config.json", R"({"enabled":true,"items":[1,2]})");
    Project.writeBuildLua(R"(
local data = beez.data.deserialize_file("config.json")
beez.data.serialize_file("out.json", data)
local copy = beez.data.deserialize_file("out.json")
local ok = data.enabled == true and copy.enabled == true and copy.items[2] == 2
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, MergeCloneGetSetAndDiff)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local a = { x = 1, nested = { y = 2 } }
local b = { nested = { z = 3 }, extra = 4 }
beez.data.merge(a, b)
local cloned = beez.data.clone(a)
cloned.extra = 99
local value = beez.data.get(a, "nested.z", 0)
local missing = beez.data.get(a, "missing.path", "fallback")
beez.data.set(a, "deep.new.path", 7)
local delta = beez.data.diff({ x = 1 }, { x = 2, added = true })
local ok = value == 3
  and missing == "fallback"
  and a.deep.new.path == 7
  and a.extra == 4
  and cloned.extra == 99
  and delta["x"].from == 1
  and delta["x"].to == 2
  and delta["added"].from == nil
  and delta["added"].to == true
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, ValidateSchema)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local schema = {
  type = "object",
  required = { "name" },
  properties = {
    name = { type = "string" },
    count = { type = "integer" }
  }
}
local ok = beez.data.validate({ name = "beez", count = 1 }, schema)
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, YamlRoundTrip)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local yaml = "name: beez\nvalues:\n  - one\n  - two\n"
local data = beez.data.deserialize_string(yaml, { type = "yaml" })
local back = beez.data.serialize_string(data, { type = "yaml" })
local ok = data.name == "beez" and data.values[2] == "two" and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, XmlRoundTrip)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local xml = '<config id="1"><item>alpha</item><item>beta</item></config>'
local data = beez.data.deserialize_string(xml, { type = "xml" })
local back = beez.data.serialize_string(data, { type = "xml" })
local ok = data.tag == "config"
  and data.attrs.id == "1"
  and data.children[1].tag == "item"
  and data.children[1].text == "alpha"
  and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, CsvRoundTrip)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local csv = "name,count\nbeez,3\n"
local data = beez.data.deserialize_string(csv, { type = "csv" })
local back = beez.data.serialize_string(data, { type = "csv" })
local ok = data[1][1] == "name"
  and data[2][1] == "beez"
  and data[2][2] == "3"
  and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, TomlRoundTrip)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local toml = 'name = "beez"\ncount = 3\n'
local data = beez.data.deserialize_string(toml, { type = "toml" })
local back = beez.data.serialize_string(data, { type = "toml" })
local ok = data.name == "beez" and data.count == 3 and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, ValidateRejectsMissingRequiredField)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "validate-step",
    phase = "test",
    scope = "code",
    run = function()
        local ok, err = pcall(function()
            beez.data.validate({}, { type = "object", required = { "name" } })
        end)
        if ok then
            error("expected validation failure")
        end
        if err:find("missing required field", 1, true) == nil then
            error("unexpected error: " .. tostring(err))
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("validate-step").has_value());
}

TEST(LuaDataApiTest, ValidateRejectsWrongTypeAndEnum)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "validate-step",
    phase = "test",
    scope = "code",
    run = function()
        local type_ok = pcall(function()
            beez.data.validate({ value = 1 }, {
                type = "object",
                properties = { value = { type = "string" } },
            })
        end)
        local enum_ok = pcall(function()
            beez.data.validate({ mode = "beta" }, {
                type = "object",
                properties = { mode = { type = "string", enum = { "alpha" } } },
            })
        end)
        if type_ok or enum_ok then
            error("expected validation failures")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("validate-step").has_value());
}

TEST(LuaDataApiTest, TomlNestedTablesRoundTrip)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local toml = [=[
title = "demo"

[owner]
name = "Ada"

[[items]]
name = "first"

[[items]]
name = "second"
]=]
local data = beez.data.deserialize_string(toml, { type = "toml" })
local back = beez.data.serialize_string(data, { type = "toml" })
local ok = data.title == "demo"
  and data.owner.name == "Ada"
  and data.items[1].name == "first"
  and data.items[2].name == "second"
  and back ~= ""
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDataApiTest, ValidateArrayItemsAndAdditionalProperties)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "validate-step",
    phase = "test",
    scope = "code",
    run = function()
        beez.data.validate({ tags = { "a", "b" } }, {
            type = "object",
            properties = {
                tags = {
                    type = "array",
                    items = { type = "string" },
                },
            },
            additionalProperties = false,
        })

        local extra_ok = pcall(function()
            beez.data.validate({ tags = { "a" }, extra = true }, {
                type = "object",
                properties = {
                    tags = { type = "array", items = { type = "string" } },
                },
                additionalProperties = false,
            })
        end)
        if extra_ok then
            error("expected additionalProperties failure")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("validate-step").has_value());
}
