#include "beez/core/glob_metadata_cache.hpp"

#include "beez/core/glob_expand.hpp"
#include "beez/core/glob_pattern.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

TEST(GlobMetadataCacheTest, ReusesStoredMatches)
{
    const auto TempRoot = std::filesystem::temp_directory_path() / "beez-glob-metadata-cache-test";
    std::filesystem::remove_all(TempRoot);
    std::filesystem::create_directories(TempRoot / "src");
    std::ofstream(TempRoot / "src" / "main.cpp") << "int main() { return 0; }\n";

    beez::core::GlobMetadataCache cache(true);
    const auto& matcher = beez::core::defaultGlobMatcher();

    const auto First = beez::core::expandGlobPatterns({"src/*.cpp"}, TempRoot, matcher, &cache);
    const auto Second = beez::core::expandGlobPatterns({"src/*.cpp"}, TempRoot, matcher, &cache);

    EXPECT_EQ(First, Second);
    EXPECT_EQ(First.size(), 1U);

    std::filesystem::remove_all(TempRoot);
}
