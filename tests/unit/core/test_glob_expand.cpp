#include "beez/core/glob_expand.hpp"
#include "beez/core/glob_pattern.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

class GlobExpandTest : public ::testing::Test
{
  protected:
    std::unique_ptr<beez::core::IGlobMatcher> matcher = beez::core::makeSimpleGlobMatcher();
};

}  // namespace

TEST_F(GlobExpandTest, ExpandsSingleLevelGlob)
{
    const auto Root = std::filesystem::temp_directory_path() / "beez_glob_expand_single";
    writeFile(Root / "src" / "main.cpp", "int main() {}\n");
    writeFile(Root / "src" / "util.cpp", "void util() {}\n");
    writeFile(Root / "README.md", "docs\n");

    const auto Files = beez::core::expandGlobPatterns({"src/*.cpp"}, Root, *matcher);

    std::filesystem::remove_all(Root);

    ASSERT_EQ(Files.size(), 2U);
    EXPECT_EQ(Files.at(0), "src/main.cpp");
    EXPECT_EQ(Files.at(1), "src/util.cpp");
}

TEST_F(GlobExpandTest, ExpandsRecursiveGlob)
{
    const auto Root = std::filesystem::temp_directory_path() / "beez_glob_expand_recursive";
    writeFile(Root / "src" / "a.cpp", "a\n");
    writeFile(Root / "src" / "nested" / "b.cpp", "b\n");

    const auto Files = beez::core::expandGlobPatterns({"src/**/*.cpp"}, Root, *matcher);

    std::filesystem::remove_all(Root);

    ASSERT_EQ(Files.size(), 2U);
    EXPECT_EQ(Files.at(0), "src/a.cpp");
    EXPECT_EQ(Files.at(1), "src/nested/b.cpp");
}

TEST_F(GlobExpandTest, ReturnsEmptyForMissingDirectory)
{
    const auto Root = std::filesystem::temp_directory_path() / "beez_glob_expand_missing";
    std::filesystem::create_directories(Root);

    const auto Files = beez::core::expandGlobPatterns({"missing/**/*.cpp"}, Root, *matcher);

    std::filesystem::remove_all(Root);

    EXPECT_TRUE(Files.empty());
}
