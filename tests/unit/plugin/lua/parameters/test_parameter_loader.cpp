#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"
#include "beez/plugin/lua/parameters/parameter_loader.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(ParameterDefinesTest, CreatesNestedTablesFromDotPaths)
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::applyParameterDefines(var, {"hallo.hund=Bernd"});

    EXPECT_EQ(var["hallo"]["hund"].get<std::string>(), "Bernd");
}

TEST(ParameterDefinesTest, LaterDefineOverridesEarlier)
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::applyParameterDefines(var, {"a=1", "a=2"});

    EXPECT_EQ(var["a"].get<std::string>(), "2");
}

TEST(ParameterDefinesTest, ConflictingPathThrows)
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    EXPECT_THROW(beez::plugin::lua::parameters::applyParameterDefines(var, {"a=1", "a.b=2"}),
                 std::runtime_error);
}

TEST(ParameterDefineArgumentTest, RejectsMissingSeparatorAndEmptyKey)
{
    using beez::plugin::lua::parameters::parseDefineArgument;
    EXPECT_THROW(parseDefineArgument("novalue"), std::runtime_error);
    EXPECT_THROW(parseDefineArgument("=value"), std::runtime_error);

    const auto Parts = parseDefineArgument("key=va=lue");
    EXPECT_EQ(Parts.at(0), "key");
    EXPECT_EQ(Parts.at(1), "va=lue");
}

TEST(ParameterLoaderTest, PropertiesLoadAsTypedLuaValues)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json",
              R"({"properties":{"hello":"world","port":8080,"debug":true}})");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"meta.json"}, Project.path(), std::nullopt, {});

    EXPECT_EQ(var["hello"].get<std::string>(), "world");
    EXPECT_EQ(var["port"].get<int>(), 8080);
    EXPECT_EQ(var["debug"].get<bool>(), true);
}

TEST(ParameterLoaderTest, ProfileDeepMergesOverProperties)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json",
              R"({
                "properties": {"server": {"host": "localhost", "port": 8080}},
                "profiles": {"dev": {"server": {"host": "127.0.0.1"}}}
              })");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"meta.json"}, Project.path(), "dev", {});

    EXPECT_EQ(var["server"]["host"].get<std::string>(), "127.0.0.1");
    EXPECT_EQ(var["server"]["port"].get<int>(), 8080);
}

TEST(ParameterLoaderTest, DefinesOverrideProfileAndProperties)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json",
              R"({
                "properties": {"hello": "base"},
                "profiles": {"dev": {"hello": "world"}}
              })");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"meta.json"}, Project.path(), "dev", {"hello=bye"});

    EXPECT_EQ(var["hello"].get<std::string>(), "bye");
}

TEST(ParameterLoaderTest, DefinesSurviveTableOverlayFromLaterFile)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "first.json", R"({"properties":{}})");
    writeFile(Project.path() / "second.json",
              R"({"properties": {"server": ["replaced", "by", "array"]}})");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"first.json"}, Project.path(), std::nullopt, {"server.port=1"});
    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"second.json"}, Project.path(), std::nullopt, {"server.port=1"});

    EXPECT_EQ(var["server"]["port"].get<std::string>(), "1");
}

TEST(ParameterLoaderTest, ExistingScalarIsNotReplacedByJsonTable)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json", R"({"properties": {"mode": {"fancy": true}}})");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"meta.json"}, Project.path(), std::nullopt, {"mode=fast"});

    EXPECT_EQ(var["mode"].get<std::string>(), "fast");
}

TEST(ParameterLoaderTest, UnknownProfileLoadsOnlyProperties)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json",
              R"({
                "properties": {"hello": "world"},
                "profiles": {"dev": {"hello": "dev-world"}}
              })");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"meta.json"}, Project.path(), "staging", {});

    EXPECT_EQ(var["hello"].get<std::string>(), "world");
}

TEST(ParameterLoaderTest, MultipleCallsMergeCumulatively)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "a.json", R"({"properties": {"one": 1}})");
    writeFile(Project.path() / "b.json", R"({"properties": {"one": 11, "two": 2}})");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();

    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"a.json"}, Project.path(), std::nullopt, {});
    beez::plugin::lua::parameters::loadParameterFiles(
        var, {"b.json"}, Project.path(), std::nullopt, {});

    EXPECT_EQ(var["one"].get<int>(), 11);
    EXPECT_EQ(var["two"].get<int>(), 2);
}

TEST(ParameterLoaderTest, InvalidInputsThrowWithSourceName)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "broken.json", R"({"properties": )");
    writeFile(Project.path() / "root.json", R"([1, 2])");
    writeFile(Project.path() / "types.json", R"({"properties": "nope"})");
    writeFile(Project.path() / "profiletypes.json", R"({"profiles": {"dev": "nope"}})");

    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::table var = lua.create_table();
    const auto& rootPath = Project.path();

    EXPECT_THROW(beez::plugin::lua::parameters::loadParameterFiles(
                     var, {"broken.json"}, rootPath, std::nullopt, {}),
                 std::runtime_error);
    EXPECT_THROW(beez::plugin::lua::parameters::loadParameterFiles(
                     var, {"missing.json"}, rootPath, std::nullopt, {}),
                 std::runtime_error);
    try
    {
        beez::plugin::lua::parameters::loadParameterFiles(
            var, {"root.json"}, rootPath, std::nullopt, {});
        FAIL() << "expected runtime_error";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_NE(std::string(error.what()).find("root.json"), std::string::npos);
    }
    EXPECT_THROW(beez::plugin::lua::parameters::loadParameterFiles(
                     var, {"types.json"}, rootPath, std::nullopt, {}),
                 std::runtime_error);
    EXPECT_THROW(beez::plugin::lua::parameters::loadParameterFiles(
                     var, {"profiletypes.json"}, rootPath, std::nullopt, {}),
                 std::runtime_error);
}

TEST(ParameterDslTest, ParametersFunctionExposesBeezVar)
{
    const beez::test::TempProject Project;
    writeFile(Project.path() / "meta.json",
              R"({
                "properties": {"greeting": "hello", "server": {"host": "localhost"}},
                "profiles": {"dev": {"greeting": "hi dev"}}
              })");
    Project.writeBuildLua(R"(
parameters("meta.json")
local ok = beez.var.greeting == "hi dev"
       and beez.var.server.host == "localhost"
       and beez.var.extra.flag == "on"
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    registry.setProfile("dev");
    beez::core::Context context(Project.path());
    context.setParameterDefines({"extra.flag=on"});

    beez::plugin::lua::LuaDslLoader loader;
    ASSERT_TRUE(loader.load(context, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
