#include "helpers/process_runner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#ifndef BEEZ_EXECUTABLE
#error "BEEZ_EXECUTABLE must be defined by CMake for integration tests"
#endif

TEST(CompletionTest, BashCompletionSuggestsProjectEntities)
{
    const auto ScriptPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
                            "scripts" / "test_bash_completion.sh";
    ASSERT_TRUE(std::filesystem::exists(ScriptPath));

    const beez::test::ProcessResult Result =
        beez::test::runShellScript(ScriptPath, {"BEEZ_EXECUTABLE=" + std::string(BEEZ_EXECUTABLE)});
    EXPECT_EQ(Result.exitCode, 0) << Result.output;
}
