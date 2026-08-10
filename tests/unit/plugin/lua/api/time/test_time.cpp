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

TEST(LuaTimeApiTest, NowAndUptimeReturnNumericStrings)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local now = tonumber(beez.time.now())
local uptime = tonumber(beez.time.uptime())
local ok = now ~= nil and now > 0 and uptime ~= nil and uptime >= 0
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaTimeApiTest, IsoReturnsUtcTimestamp)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("check", "echo " .. beez.time.iso())
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    const auto* shell = beez::test::shellActionAt(*Found, 0);
    ASSERT_NE(shell, nullptr);
    EXPECT_GE(shell->command.size(), 6U);
    EXPECT_EQ(shell->command.back(), 'Z');
    EXPECT_NE(shell->command.find('T'), std::string::npos);
}
