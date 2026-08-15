#include "beez/plugin/lua/dsl/reqpack_beez_plugin_catalog.hpp"

#include <gtest/gtest.h>

// NOLINTBEGIN(bugprone-unchecked-optional-access,misc-include-cleaner,readability-identifier-naming)

TEST(ReqpackBeezPluginCatalogTest, StoresFindsAndAddsPlugins)
{
    beez::plugin::lua::ReqpackBeezPluginCatalog catalog;

    const beez::plugin::lua::BeezPluginRef plugin {
        .organization = "coditary",
        .name = "demo",
        .path = "./plugins/coditary/demo",
        .version = std::string {"1.0.0"},
    };

    catalog.set({plugin});
    ASSERT_FALSE(catalog.empty());

    const auto found = catalog.find("coditary", "demo");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "demo");

    EXPECT_FALSE(catalog.find("coditary", "missing").has_value());

    catalog.add({.organization = "coditary",
                 .name = "other",
                 .path = "./plugins/coditary/other",
                 .version = std::string {"1.0.0"}});
    EXPECT_TRUE(catalog.find("coditary", "other").has_value());
}

// NOLINTEND(bugprone-unchecked-optional-access,misc-include-cleaner,readability-identifier-naming)
