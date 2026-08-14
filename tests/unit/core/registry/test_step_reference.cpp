#include "beez/core/registry/step_reference.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(StepReferenceTest, FormatsQualifiedReference)
{
    EXPECT_EQ(beez::core::formatQualifiedStepRef("coditary", "clang-build", "compile:code"),
              "coditary/clang-build:compile:code");
}

TEST(StepReferenceTest, FormatsShortPluginReference)
{
    EXPECT_EQ(beez::core::formatShortPluginStepRef("clang-tidy", "check"), "clang-tidy:check");
}

TEST(StepReferenceTest, ParsesQualifiedReferenceWithColonInStepName)
{
    const auto Parsed = beez::core::parseQualifiedStepRef("coditary/clang-build:compile:code");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->organization, "coditary");
    EXPECT_EQ(Parsed->plugin, "clang-build");
    EXPECT_EQ(Parsed->stepName, "compile:code");
}

TEST(StepReferenceTest, ParsesQualifiedReferenceWithSimpleStepName)
{
    const auto Parsed = beez::core::parseQualifiedStepRef("coditary/clang-tidy:check");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->organization, "coditary");
    EXPECT_EQ(Parsed->plugin, "clang-tidy");
    EXPECT_EQ(Parsed->stepName, "check");
}

TEST(StepReferenceTest, RejectsPlainStepNameAsQualifiedReference)
{
    EXPECT_FALSE(beez::core::parseQualifiedStepRef("compile:code").has_value());
    EXPECT_FALSE(beez::core::parseQualifiedStepRef("check").has_value());
}

TEST(StepReferenceTest, ParsesShortPluginReference)
{
    const auto Parsed = beez::core::parseShortPluginStepRef("clang-tidy:check");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->first, "clang-tidy");
    EXPECT_EQ(Parsed->second, "check");
}

TEST(StepReferenceTest, SplitsVersionSuffix)
{
    const auto [Base, Version] = beez::core::splitStepReferenceVersion("clang-build:compile@1.0.0");
    EXPECT_EQ(Base, "clang-build:compile");
    ASSERT_TRUE(Version.has_value());
    EXPECT_EQ(*Version, "1.0.0");
}

TEST(StepReferenceTest, ParsesQualifiedReferenceWithVersionSuffix)
{
    const auto Parsed = beez::core::parseQualifiedStepRef("coditary/clang-build:compile@1.0.0");
    ASSERT_TRUE(Parsed.has_value());
    EXPECT_EQ(Parsed->organization, "coditary");
    EXPECT_EQ(Parsed->plugin, "clang-build");
    EXPECT_EQ(Parsed->stepName, "compile");
    ASSERT_TRUE(Parsed->version.has_value());
    EXPECT_EQ(*Parsed->version, "1.0.0");
}

TEST(StepReferenceTest, ExtractsActionNameWithoutScopeSuffix)
{
    EXPECT_EQ(beez::core::stepActionName("compile:code"), "compile");
    EXPECT_EQ(beez::core::stepActionName("link:debug"), "link");
    EXPECT_EQ(beez::core::stepActionName("check"), "check");
}
