#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

namespace
{

bool loadScript(const beez::test::TempProject& project, beez::core::Registry& registry)
{
    const beez::core::Context Ctx(project.path());
    beez::plugin::lua::LuaDslLoader loader;
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(LuaDateApiTest, FormatUsesCurrentYear)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local year = tostring(beez.date.info().year)
local formatted = beez.date.format("%Y")
local ok = formatted == year
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDateApiTest, InfoReturnsExpectedFields)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local info = beez.date.info()
local ok = info.year > 0 and info.month >= 1 and info.month <= 12 and info.day >= 1
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}

TEST(LuaDateApiTest, UtcReturnsZuluSuffix)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("check", "echo " .. beez.date.utc(0))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    const auto* shell = beez::test::shellActionAt(Found, 0);
    ASSERT_NE(shell, nullptr);
    EXPECT_EQ(shell->command.back(), 'Z');
    EXPECT_NE(shell->command.find("1970"), std::string::npos);
}

TEST(LuaDateApiTest, CurrentEpochIsPositive)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("check", "echo " .. tostring(beez.date.epoch() > 0))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(Found, 0, "echo true");
}
