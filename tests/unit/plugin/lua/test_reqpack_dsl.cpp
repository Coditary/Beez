#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include "helpers/temp_project.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{

bool loadScript(const beez::test::TempProject& project,
                beez::core::Registry& registry,
                beez::plugin::lua::LuaDslLoader& loader)
{
    const beez::core::Context Ctx(project.path());
    return loader.load(Ctx, registry);
}

}  // namespace

TEST(ReqPackDslTest, ParsesStringAndVersionedPackages)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    sys = { "make", "cmake" },
    npm = {
        "typescript",
        { name = "eslint", version = "3.2.1" },
    },
}
)");

    beez::core::Registry registry;
    beez::plugin::lua::LuaDslLoader loader;
    ASSERT_TRUE(loadScript(Project, registry, loader));

    const auto& manifest = loader.reqpackManifest();
    ASSERT_EQ(manifest.plugins.size(), 2U);
    ASSERT_TRUE(manifest.plugins.contains("sys"));
    ASSERT_TRUE(manifest.plugins.contains("npm"));

    const auto& sysPackages = manifest.plugins.at("sys");
    ASSERT_EQ(sysPackages.size(), 2U);
    EXPECT_EQ(sysPackages.at(0).name, "make");
    EXPECT_FALSE(sysPackages.at(0).version.has_value());

    const auto& npmPackages = manifest.plugins.at("npm");
    ASSERT_EQ(npmPackages.size(), 2U);
    EXPECT_EQ(npmPackages.at(0).name, "typescript");
    EXPECT_EQ(npmPackages.at(1).name, "eslint");
    ASSERT_TRUE(npmPackages.at(1).version.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*npmPackages.at(1).version, "3.2.1");
}

TEST(ReqPackDslTest, RejectsInvalidPackageEntry)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    npm = { 42 },
}
)");

    beez::core::Registry registry;
    beez::plugin::lua::LuaDslLoader loader;
    EXPECT_FALSE(loadScript(Project, registry, loader));
}

TEST(ReqPackDslTest, MergesRepeatedReqpackBlocks)
{
    const beez::test::TempProject Project;
    Project.writeBuildLua(R"(
reqpack {
    sys = { "make" },
}

reqpack {
    npm = { "eslint" },
}
)");

    beez::core::Registry registry;
    beez::plugin::lua::LuaDslLoader loader;
    ASSERT_TRUE(loadScript(Project, registry, loader));

    const auto& manifest = loader.reqpackManifest();
    ASSERT_EQ(manifest.plugins.size(), 2U);
    ASSERT_TRUE(manifest.plugins.contains("sys"));
    ASSERT_TRUE(manifest.plugins.contains("npm"));
    EXPECT_EQ(manifest.plugins.at("sys").at(0).name, "make");
    EXPECT_EQ(manifest.plugins.at("npm").at(0).name, "eslint");
}
