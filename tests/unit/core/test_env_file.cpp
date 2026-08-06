#include "beez/core/env_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream stream(path);
    stream << content;
}

}  // namespace

TEST(EnvFileTest, LookupReturnsValueFromDotEnv)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_lookup";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env", "MY_ENV=hello-from-dotenv\n");

    const beez::core::EnvFile Env(Directory / ".env");
    const auto Value = Env.lookup("MY_ENV");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(Value.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Value, "hello-from-dotenv");
}

TEST(EnvFileTest, LookupReturnsEmptyOptionalWhenMissing)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_missing";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env", "OTHER=value\n");

    const beez::core::EnvFile Env(Directory / ".env");
    const auto Value = Env.lookup("MISSING");

    std::filesystem::remove_all(Directory);

    EXPECT_FALSE(Value.has_value());
}

TEST(EnvFileTest, MissingDotEnvFileFallsBackToProcessEnvironment)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_no_file";
    std::filesystem::create_directories(Directory);

    const beez::core::EnvFile Env(Directory / ".env");
    const auto Value = Env.lookup("PATH");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(Value.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_FALSE(Value->empty());
}

TEST(EnvFileTest, ParsesQuotedValuesAndSkipsComments)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_parse";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env",
              "# comment\n"
              "FOO=\"quoted value\"\n"
              "BAR='single'\n"
              "export BAZ=from-export\n");

    const beez::core::EnvFile Env(Directory / ".env");

    const auto Foo = Env.lookup("FOO");
    const auto Bar = Env.lookup("BAR");
    const auto Baz = Env.lookup("BAZ");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(Foo.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Foo, "quoted value");
    ASSERT_TRUE(Bar.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Bar, "single");
    ASSERT_TRUE(Baz.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Baz, "from-export");
}

TEST(EnvFileTest, DotEnvTakesPrecedenceOverProcessEnvironment)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_precedence";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env", "PATH=from-dotenv\n");

    const beez::core::EnvFile Env(Directory / ".env");
    const auto Value = Env.lookup("PATH");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(Value.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Value, "from-dotenv");
}

TEST(EnvFileTest, ParsesValuesContainingEqualsSign)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_equals";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env", "CONNECTION=host=db;port=5432\n");

    const beez::core::EnvFile Env(Directory / ".env");
    const auto Value = Env.lookup("CONNECTION");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(Value.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*Value, "host=db;port=5432");
}

TEST(EnvFileTest, LookupIsLazyAndOnlyParsesOnce)
{
    const auto Directory = std::filesystem::temp_directory_path() / "beez_env_test_lazy";
    std::filesystem::create_directories(Directory);
    writeFile(Directory / ".env", "FIRST=one\n");

    const beez::core::EnvFile Env(Directory / ".env");
    const auto First = Env.lookup("FIRST");
    writeFile(Directory / ".env", "FIRST=two\n");
    const auto Second = Env.lookup("FIRST");

    std::filesystem::remove_all(Directory);

    ASSERT_TRUE(First.has_value());
    ASSERT_TRUE(Second.has_value());
    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above
    EXPECT_EQ(*First, "one");
    EXPECT_EQ(*Second, "one");
    // NOLINTEND(bugprone-unchecked-optional-access)
}
