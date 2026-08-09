#include "beez/core/cache/include_fingerprint.hpp"

#include "beez/core/cache/content_hash.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(IncludeFingerprintTest, ChangesWhenIncludedHeaderChanges)
{
    const auto Root = std::filesystem::temp_directory_path() / "beez_include_fingerprint_test";
    writeFile(Root / "include" / "widget.hpp", "#pragma once\nstruct Widget {};\n");
    writeFile(Root / "src" / "main.cpp", R"(
#include "include/widget.hpp"
int main() {}
)");

    const auto Hasher = beez::core::makeContentHasher({});
    const std::string Before =
        beez::core::includeTreeFingerprint(Root / "src" / "main.cpp", Root, *Hasher);

    writeFile(Root / "include" / "widget.hpp", "#pragma once\nstruct Widget { int value; };\n");
    const std::string After =
        beez::core::includeTreeFingerprint(Root / "src" / "main.cpp", Root, *Hasher);

    std::filesystem::remove_all(Root);

    EXPECT_NE(Before, After);
}

TEST(IncludeFingerprintTest, StableWhenOnlyUnrelatedFilesChange)
{
    const auto Root = std::filesystem::temp_directory_path() / "beez_include_fingerprint_stable";
    writeFile(Root / "include" / "widget.hpp", "#pragma once\nstruct Widget {};\n");
    writeFile(Root / "src" / "main.cpp", R"(
#include "include/widget.hpp"
int main() {}
)");
    writeFile(Root / "other.cpp", "int other() { return 0; }\n");

    const auto Hasher = beez::core::makeContentHasher({});
    const std::string First =
        beez::core::includeTreeFingerprint(Root / "src" / "main.cpp", Root, *Hasher);
    const std::string Second =
        beez::core::includeTreeFingerprint(Root / "src" / "main.cpp", Root, *Hasher);

    std::filesystem::remove_all(Root);

    EXPECT_EQ(First, Second);
}
