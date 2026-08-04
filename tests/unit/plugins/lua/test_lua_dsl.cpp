#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"

#include "helpers/temp_project.hpp"
#include "helpers/test_helpers.hpp"

#include <gtest/gtest.h>

#include <optional>
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

TEST(LuaDslTest, LoadsOrphanTaskFromStringForm)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("clean", "rm -fr app.o")
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findTask("clean");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->run, "rm -fr app.o");
    EXPECT_TRUE(Found->isOrphan());
}

TEST(LuaDslTest, LoadsPhaseBoundTaskFromTableForm)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("doxygen", { phase = "generate", scope = "docs", run = "doxygen Doxyfile" })
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = registry.findTask("doxygen");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    EXPECT_EQ(Found->phase, std::optional<std::string> {"generate"});
    EXPECT_EQ(Found->scope, std::optional<std::string> {"docs"});
    EXPECT_EQ(Found->run, "doxygen Doxyfile");
}

TEST(LuaDslTest, LoadsSequentialWorkflow)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("build", {
    { phase = "generate", scope = "code" },
    { phase = "compile", scope = "code" },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireWorkflow(registry, "build");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->steps.size(), 2U);
    beez::test::expectSequentialStep(Found->steps[0], "generate", "code");
    beez::test::expectSequentialStep(Found->steps[1], "compile", "code");
}

TEST(LuaDslTest, LoadsWorkflowWithParallelStep)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
workflow("ci", {
    { parallel = {
        { phase = "generate", scope = "docs" },
        { phase = "generate", scope = "code" },
    }},
    { phase = "compile", scope = "code" },
})
)");

    beez::core::Registry registry;
    ASSERT_TRUE(loadScript(Project, registry));

    const auto Found = beez::test::requireWorkflow(registry, "ci");
    ASSERT_TRUE(Found.has_value());
    if (!Found)
    {
        return;
    }
    ASSERT_EQ(Found->steps.size(), 2U);
    beez::test::expectParallelStep(Found->steps[0], {{"generate", "docs"}, {"generate", "code"}});
    beez::test::expectSequentialStep(Found->steps[1], "compile", "code");
}

TEST(LuaDslTest, ReturnsFalseForSyntaxError)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua("this is not valid lua {{{");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("clean").has_value());
}

TEST(LuaDslTest, ReturnsFalseWhenTaskTableMissingRun)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
task("broken", { phase = "generate", scope = "docs" })
)");

    beez::core::Registry registry;
    EXPECT_FALSE(loadScript(Project, registry));
    EXPECT_FALSE(registry.findTask("broken").has_value());
}
