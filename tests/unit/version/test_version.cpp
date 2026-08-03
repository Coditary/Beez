#include "beez/version.hpp"

#include <gtest/gtest.h>

TEST(VersionTest, MajorVersion)
{
    EXPECT_EQ(beez::version::MajorVersion, 0);
}

TEST(VersionTest, MinorVersion)
{
    EXPECT_EQ(beez::version::MinorVersion, 1);
}

TEST(VersionTest, PatchVersion)
{
    EXPECT_EQ(beez::version::PatchVersion, 0);
}

TEST(VersionTest, VersionString)
{
    EXPECT_STREQ(beez::version::VersionString, "0.1.0");
}
