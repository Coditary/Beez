#include "beez/logging/worker_output_format.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace
{

[[nodiscard]] std::vector<std::string> toStrings(const std::vector<std::string_view>& views)
{
    std::vector<std::string> strings;
    strings.reserve(views.size());
    for (const std::string_view View : views)
    {
        strings.emplace_back(View);
    }

    return strings;
}

}  // namespace

TEST(WorkerOutputFormatTest, DoesNotWrapWhenTerminalWidthIsZero)
{
    const auto Segments =
        toStrings(beez::logging::splitWorkerOutputLine("abcdefghijklmnopqrstuvwxyz", 0));
    ASSERT_EQ(Segments.size(), 1U);
    EXPECT_EQ(Segments[0], "abcdefghijklmnopqrstuvwxyz");
}

TEST(WorkerOutputFormatTest, KeepsShortLineOnSingleSegment)
{
    const std::size_t Width = beez::logging::WorkerOutputPrefix.size() + 10;
    const auto Segments = toStrings(beez::logging::splitWorkerOutputLine("short line", Width));
    ASSERT_EQ(Segments.size(), 1U);
    EXPECT_EQ(Segments[0], "short line");
}

TEST(WorkerOutputFormatTest, WrapsLongLineWithPrefixBudget)
{
    const std::size_t Width = beez::logging::WorkerOutputPrefix.size() + 5;
    const auto Segments = toStrings(beez::logging::splitWorkerOutputLine("abcdefghij", Width));
    ASSERT_EQ(Segments.size(), 2U);
    EXPECT_EQ(Segments[0], "abcde");
    EXPECT_EQ(Segments[1], "fghij");
}

TEST(WorkerOutputFormatTest, ReturnsNoSegmentsForEmptyLine)
{
    const auto Segments = beez::logging::splitWorkerOutputLine("", 80);
    EXPECT_TRUE(Segments.empty());
}
