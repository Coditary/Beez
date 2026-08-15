#include "beez/plugin/lua/api/char/quote.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(CharQuoteTest, ShellQuoteWrapsPlainStrings)
{
    EXPECT_EQ(beez::plugin::lua::shellQuote("hello"), "'hello'");
}

TEST(CharQuoteTest, ShellQuoteHandlesEmptyString)
{
    EXPECT_EQ(beez::plugin::lua::shellQuote(""), "''");
}

TEST(CharQuoteTest, ShellQuoteEscapesApostrophes)
{
    EXPECT_EQ(beez::plugin::lua::shellQuote("it's fine"), "'it'\\''s fine'");
}

TEST(CharQuoteTest, ShellQuoteEscapesMultipleApostrophes)
{
    EXPECT_EQ(beez::plugin::lua::shellQuote("a'b'c"), "'a'\\''b'\\''c'");
}
