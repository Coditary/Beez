#include "beez/core/text_table.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(TextTableTest, AutoSizesColumnsToContent)
{
    beez::core::TextTable table({"Key", "Kind", "Details"});
    table.addRow({"algorithm", "enum", "5 values"});
    table.addRow({"seed", "number", "uint32, >= 0, default 0"});

    const std::string Formatted = table.format();

    EXPECT_NE(Formatted.find("Key"), std::string::npos);
    EXPECT_NE(Formatted.find("Details"), std::string::npos);
    EXPECT_NE(Formatted.find("algorithm"), std::string::npos);
    EXPECT_NE(Formatted.find("uint32, >= 0, default 0"), std::string::npos);
    EXPECT_LT(Formatted.find("algorithm"), Formatted.find("seed"));
    EXPECT_LT(Formatted.find('\n'), Formatted.find("algorithm"));
}

TEST(TextTableTest, SupportsCustomColumnSpacing)
{
    beez::core::TextTable table({"A", "B"});
    table.setColumnSpacing(4);
    table.addRow({"x", "yyyy"});

    const std::string Formatted = table.format();
    EXPECT_NE(Formatted.find("A    B"), std::string::npos);
    EXPECT_NE(Formatted.find("x    yyyy"), std::string::npos);
}

TEST(TextTableTest, EmptyHeadersReturnEmptyString)
{
    const beez::core::TextTable Table({});
    EXPECT_TRUE(Table.format().empty());
}
