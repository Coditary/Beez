#include "beez/core/registry/task_reference.hpp"

#include <gtest/gtest.h>

TEST(TaskReferenceTest, ParsesPluginTaskReference)
{
    const auto Parsed = beez::core::parsePluginTaskReference("coditary/format:format");
    EXPECT_EQ(Parsed.organization, "coditary");
    EXPECT_EQ(Parsed.plugin, "format");
    EXPECT_EQ(Parsed.taskName, "format");
}

TEST(TaskReferenceTest, DetectsPluginTaskReferenceShape)
{
    EXPECT_TRUE(beez::core::looksLikePluginTaskReference("coditary/format:format"));
    EXPECT_FALSE(beez::core::looksLikePluginTaskReference("echo hello"));
    EXPECT_FALSE(beez::core::looksLikePluginTaskReference("format"));
}
