#include "beez/core/config/config_schema.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

namespace
{

std::string requireFormattedOptions(const std::string& path)
{
    const auto Output = beez::core::formatConfigOptions(path);
    EXPECT_TRUE(Output.has_value());
    if (!Output.has_value())
    {
        return {};
    }

    return *Output;
}

}  // namespace

TEST(ConfigSchemaTest, RootShowsTableWithChildKinds)
{
    const std::string Output = requireFormattedOptions("");
    EXPECT_NE(Output.find("=== config ==="), std::string::npos);
    EXPECT_NE(Output.find("Kind: table"), std::string::npos);
    EXPECT_NE(Output.find("performance"), std::string::npos);
    EXPECT_NE(Output.find("cache"), std::string::npos);
    EXPECT_NE(Output.find("env"), std::string::npos);
    EXPECT_NE(Output.find("ui"), std::string::npos);
}

TEST(ConfigSchemaTest, EnvVarsShowsStringMapKind)
{
    const std::string Output = requireFormattedOptions("env");
    EXPECT_NE(Output.find("vars"), std::string::npos);
    EXPECT_NE(Output.find("map"), std::string::npos);
}

TEST(ConfigSchemaTest, NestedObjectShowsChildKinds)
{
    const std::string Output = requireFormattedOptions("cache.hash");
    EXPECT_NE(Output.find("=== cache.hash ==="), std::string::npos);
    EXPECT_NE(Output.find("Kind: table"), std::string::npos);
    EXPECT_NE(Output.find("algorithm"), std::string::npos);
    EXPECT_NE(Output.find("enum"), std::string::npos);
    EXPECT_NE(Output.find("seed"), std::string::npos);
    EXPECT_NE(Output.find("number"), std::string::npos);
    EXPECT_NE(Output.find("uint32"), std::string::npos);
}

TEST(ConfigSchemaTest, EnumLeafListsValuesSection)
{
    const std::string Output = requireFormattedOptions("cache.hash.algorithm");
    EXPECT_NE(Output.find("=== cache.hash.algorithm ==="), std::string::npos);
    EXPECT_NE(Output.find("Kind: enum"), std::string::npos);
    EXPECT_NE(Output.find("Value"), std::string::npos);
    EXPECT_NE(Output.find("fnv1a64"), std::string::npos);
    EXPECT_NE(Output.find("crc32"), std::string::npos);
    EXPECT_NE(Output.find("sdbm"), std::string::npos);
}

TEST(ConfigSchemaTest, NumberLeafShowsRangeAndDefault)
{
    const std::string Output = requireFormattedOptions("cache.hash.seed");
    EXPECT_NE(Output.find("=== cache.hash.seed ==="), std::string::npos);
    EXPECT_NE(Output.find("Kind: number"), std::string::npos);
    EXPECT_NE(Output.find("Type"), std::string::npos);
    EXPECT_NE(Output.find("uint32"), std::string::npos);
    EXPECT_NE(Output.find("Range"), std::string::npos);
    EXPECT_NE(Output.find(">= 0"), std::string::npos);
    EXPECT_NE(Output.find("Default"), std::string::npos);
    EXPECT_NE(Output.find('0'), std::string::npos);
}

TEST(ConfigSchemaTest, CompressionLevelShowsBoundedInteger)
{
    const std::string Output = requireFormattedOptions("cache.compress.level");
    EXPECT_NE(Output.find("Kind: number"), std::string::npos);
    EXPECT_NE(Output.find("<= 9"), std::string::npos);
    EXPECT_NE(Output.find("Default"), std::string::npos);
    EXPECT_NE(Output.find('6'), std::string::npos);
}

TEST(ConfigSchemaTest, CompressionModeListsAllowedValues)
{
    const std::string Output = requireFormattedOptions("cache.compress.mode");
    EXPECT_NE(Output.find("=== cache.compress.mode ==="), std::string::npos);
    EXPECT_NE(Output.find("Kind: enum"), std::string::npos);
    EXPECT_NE(Output.find("never"), std::string::npos);
    EXPECT_NE(Output.find("always"), std::string::npos);
    EXPECT_NE(Output.find("auto"), std::string::npos);
}

TEST(ConfigSchemaTest, UnknownPathReturnsNullopt)
{
    EXPECT_FALSE(beez::core::formatConfigOptions("cache.unknown").has_value());
    EXPECT_FALSE(beez::core::formatConfigOptions("env.vars.FOO").has_value());
}

TEST(ConfigSchemaTest, ListsRootCompletions)
{
    const auto Completions = beez::core::listConfigOptionCompletions("");
    ASSERT_FALSE(Completions.empty());
    EXPECT_NE(std::ranges::find(Completions, "performance"), Completions.end());
    EXPECT_NE(std::ranges::find(Completions, "cache"), Completions.end());
}

TEST(ConfigSchemaTest, ListsNestedCompletionsWithPartialPrefix)
{
    const auto Completions = beez::core::listConfigOptionCompletions("performance.cache");
    ASSERT_EQ(Completions.size(), 2U);
    EXPECT_EQ(Completions.at(0), "performance.cache_fs_metadata");
    EXPECT_EQ(Completions.at(1), "performance.cache_write_strategy");
}

TEST(ConfigSchemaTest, ListsPerformanceWriteStrategyEnumPath)
{
    const auto Completions = beez::core::listConfigOptionCompletions("performance.cache_w");
    ASSERT_EQ(Completions.size(), 1U);
    EXPECT_EQ(Completions.front(), "performance.cache_write_strategy");
}

TEST(ConfigSchemaTest, ExpandsExactObjectPathToChildren)
{
    const auto Completions = beez::core::listConfigOptionCompletions("performance");
    ASSERT_GE(Completions.size(), 3U);
    EXPECT_NE(std::ranges::find(Completions, "performance.cache_write_strategy"),
              Completions.end());
    EXPECT_NE(std::ranges::find(Completions, "performance.max_threads"), Completions.end());
}

TEST(ConfigSchemaTest, ExpandsTrailingDotToChildren)
{
    const auto Completions = beez::core::listConfigOptionCompletions("cache.");
    ASSERT_FALSE(Completions.empty());
    EXPECT_NE(std::ranges::find(Completions, "cache.hash"), Completions.end());
}
