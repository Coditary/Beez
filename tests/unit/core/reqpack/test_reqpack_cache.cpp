#include "beez/core/reqpack/cache.hpp"
#include "beez/core/reqpack/types.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

namespace
{

beez::core::ReqPackManifest npmManifest()
{
    beez::core::ReqPackManifest manifest;
    manifest.plugins.emplace("npm",
                             std::vector<beez::core::ReqPackPackage> {
                                 {.name = "eslint", .version = "3.2.1"},
                             });
    return manifest;
}

}  // namespace

TEST(ReqPackCacheTest, PluginFingerprintIsStable)
{
    const std::vector<beez::core::ReqPackPackage> Packages {
        {.name = "eslint", .version = "3.2.1"},
        {.name = "vitest"},
    };
    EXPECT_EQ(beez::core::pluginFingerprint(Packages), beez::core::pluginFingerprint(Packages));
}

TEST(ReqPackCacheTest, PluginFingerprintChangesWhenPackagesChange)
{
    const std::vector<beez::core::ReqPackPackage> Before {{.name = "eslint"}};
    const std::vector<beez::core::ReqPackPackage> After {{.name = "eslint", .version = "3.2.1"}};
    EXPECT_NE(beez::core::pluginFingerprint(Before), beez::core::pluginFingerprint(After));
}

TEST(ReqPackCacheTest, FiltersUncachedPluginsAndPersistsCache)
{
    const auto TempRoot = std::filesystem::temp_directory_path() / "beez_reqpack_cache_test";
    std::filesystem::remove_all(TempRoot);
    std::filesystem::create_directories(TempRoot);

    const auto Manifest = npmManifest();
    const auto Uncached = beez::core::filterUncachedPlugins(Manifest, TempRoot);
    ASSERT_EQ(Uncached.size(), 1U);
    EXPECT_TRUE(Uncached.contains("npm"));

    beez::core::updatePluginCache("npm", Manifest.plugins.at("npm"), TempRoot);

    const auto Cached = beez::core::filterUncachedPlugins(Manifest, TempRoot);
    EXPECT_TRUE(Cached.empty());

    std::filesystem::remove_all(TempRoot);
}
