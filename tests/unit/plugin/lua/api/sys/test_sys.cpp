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

TEST(LuaSysApiTest, CpuCountsArePositive)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local ok = beez.sys.cpu_cores() > 0 and beez.sys.cpu_threads() > 0
    and beez.sys.cpu_threads() >= beez.sys.cpu_cores()
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaSysApiTest, RamValuesAreConsistent)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local ok = beez.sys.ram_total() > 0 and beez.sys.ram_free() > 0
    and beez.sys.ram_total() >= beez.sys.ram_free()
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaSysApiTest, PathAndProcessInfoAreAvailable)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local sep = beez.sys.path_separator()
local ok = sep == "/" or sep == "\\"
    and #beez.sys.cwd() > 0
    and #beez.sys.tmp_dir() > 0
    and beez.sys.pid() > 0
    and #beez.sys.user() > 0
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaSysApiTest, IsTtyReturnsBoolean)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
local value = beez.sys.is_tty()
local ok = value == true or value == false
task("check", "echo " .. tostring(ok))
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireTask(registry, "check");
    ASSERT_TRUE(Found.has_value());
    beez::test::expectShellCommand(*Found, 0, "echo true");
}

TEST(LuaSysApiTest, SysApiWorksInsideStepCallback)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
step({
    name = "sys-step",
    phase = "test",
    scope = "code",
    run = function()
        if beez.sys.cpu_cores() <= 0 then
            error("expected positive cpu core count")
        end
        return 0
    end,
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));
    EXPECT_TRUE(registry.findStep("sys-step").has_value());
}
